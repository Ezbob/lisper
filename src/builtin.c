#include "builtin.h"
#include "environment.h"
#include "grammar.h"
#include "value/constructors.h"
#include "value/lfile.h"
#include "value/lvalue.h"
#include "value/print.h"
#include "value/transformers.h"
#include "lisper_internal.h"
#include <math.h>
#include <stdlib.h>


#define LGETCELL(v, celln) (v->val.list.cells[(celln)])
#define LCELLCOUNT(v) (v->val.list.count)

#define UNUSED(x) (void)(x)

#define LIS_NUM(type) (type == LVAL_INT || type == LVAL_FLOAT)

#define LASSERT(mp, lval, cond, fmt, ...)                                                \
  if (!(cond)) {                                                                         \
    struct lvalue *err = lvalue_err(mp, fmt, ##__VA_ARGS__);                             \
    lvalue_del(mp, lval);                                                                \
    return err;                                                                          \
  }

#define LNUM_LEAST_ARGS(mp, sym, funcname, numargs)                                      \
  LASSERT(mp, sym, LCELLCOUNT(sym) >= numargs,                                           \
          "Wrong number of arguments parsed to '%s'. Expected at least %lu "             \
          "argument(s); got %lu. ",                                                      \
          funcname, numargs, LCELLCOUNT(sym))

#define LNUM_ARGS(mp, lval, func_name, numargs)                                          \
  LASSERT(mp, lval, LCELLCOUNT(lval) == numargs,                                         \
          "Wrong number of arguments parsed to '%s'. Expected at exactly %lu "           \
          "argument(s); got %lu. ",                                                      \
          func_name, numargs, LCELLCOUNT(lval))

#define LNOT_EMPTY_QEXPR(mp, lval, func_name, i)                                         \
  LASSERT(mp, lval,                                                                      \
          LGETCELL(lval, i)->type == LVAL_QEXPR && LCELLCOUNT(LGETCELL(lval, i)) > 0,    \
          "Empty %s parsed to '%s'.", ltype_name(LGETCELL(lval, i)->type, func_name))

#define LARG_TYPE(mp, lval, func_name, i, expected)                                      \
  LASSERT(mp, lval, LGETCELL(lval, i)->type == expected,                                 \
          "Wrong type of argument parsed to '%s' at argument position %lu. Expected "    \
          "argument to be of type '%s'; got '%s'.",                                      \
          func_name, (i + 1), ltype_name(expected), ltype_name(LGETCELL(lval, i)->type))

#define LTWO_ARG_TYPES(mp, lval, func_name, i, first_expect, second_expect)              \
  LASSERT(mp, lval,                                                                      \
          LGETCELL(lval, i)->type == first_expect ||                                     \
              LGETCELL(lval, i)->type == second_expect,                                  \
          "Wrong type of argument parsed to '%s'. Expected argument to be of type '%s' " \
          "or '%s'; got '%s'.",                                                          \
          func_name, ltype_name(first_expect), ltype_name(second_expect),                \
          ltype_name(LGETCELL(lval, i)->type))

#define LENV_BUILTIN(name) lenvironment_add_builtin(e, #name, builtin_##name)
#define LENV_SYMBUILTIN(sym, name) lenvironment_add_builtin(e, sym, builtin_##name)

#define LMATH_TYPE_CHECK(mp, lval, sym)                                                  \
  do {                                                                                   \
    enum ltype expected_arg_type = LGETCELL(lval, 0)->type;                              \
    LASSERT(mp, lval, LIS_NUM(expected_arg_type),                                        \
            "Cannot operate on argument at position %i. Non-number type '%s' parsed to " \
            "operator '%s'.",                                                            \
            1, ltype_name(expected_arg_type), sym);                                      \
    for (int i = 1; i < LCELLCOUNT(lval); i++) {                                         \
      struct lvalue *curr = LGETCELL(lval, i);                                           \
      LASSERT(mp, lval, expected_arg_type == curr->type,                                 \
              "Argument type mismatch. Expected argument at position %lu to be of type " \
              "'%s'; got type '%s'.",                                                    \
              i + 1, ltype_name(expected_arg_type), ltype_name(curr->type));             \
    }                                                                                    \
  } while (0)

/* env preallocated sizes */
const size_t lambda_env_prealloc = 50;
const size_t fun_env_prealloc = 200;

/* * math builtins * */

struct lvalue *builtin_add(struct linterpreter *intp, struct lvalue *in) {
  do {
    enum ltype expected_arg_type = LGETCELL(in, 0)->type;
    if (!((expected_arg_type == LVAL_INT || expected_arg_type == LVAL_FLOAT))) {
      struct lvalue *err = lvalue_err(
          intp->lvalue_mp,
          "Cannot operate on argument at position %i. Non-number type '%s' parsed to "
          "operator '%s'.",
          1, ltype_name(expected_arg_type), "+");
      lvalue_del(intp->lvalue_mp, in);
      return err;
    };
    for (size_t i = 1; i < LCELLCOUNT(in); i++) {
      struct lvalue *curr = LGETCELL(in, i);
      if (!(expected_arg_type == curr->type)) {
        struct lvalue *err = lvalue_err(
            intp->lvalue_mp,
            "Argument type mismatch. Expected argument at position %lu to be of type "
            "'%s'; got type '%s'.",
            i + 1, ltype_name(expected_arg_type), ltype_name(curr->type));
        lvalue_del(intp->lvalue_mp, in);
        return err;
      };
    }
  } while (0);
  struct lvalue *res = lvalue_pop(intp->lvalue_mp, in, 0);

  if (res->type == LVAL_INT) {
    while (LCELLCOUNT(in) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, in, 0);
      res->val.intval += b->val.intval;
      lvalue_del(intp->lvalue_mp, b);
    }
  } else {
    while (LCELLCOUNT(in) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, in, 0);
      res->val.floatval += b->val.floatval;
      lvalue_del(intp->lvalue_mp, b);
    }
  }

  lvalue_del(intp->lvalue_mp, in);
  return res;
}

struct lvalue *builtin_sub(struct linterpreter *intp, struct lvalue *a) {
  LMATH_TYPE_CHECK(intp->lvalue_mp, a, "-");
  struct lvalue *res = lvalue_pop(intp->lvalue_mp, a, 0);

  if (res->type == LVAL_INT) {
    if (LCELLCOUNT(a) == 0) {
      res->val.intval = (-res->val.intval);
    } else {
      while (LCELLCOUNT(a) > 0) {
        struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
        res->val.intval -= b->val.intval;
        lvalue_del(intp->lvalue_mp, b);
      }
    }
  } else {
    if (LCELLCOUNT(a) == 0) {
      res->val.floatval = (-res->val.floatval);
    } else {
      while (LCELLCOUNT(a) > 0) {
        struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
        res->val.floatval -= b->val.floatval;
        lvalue_del(intp->lvalue_mp, b);
      }
    }
  }

  lvalue_del(intp->lvalue_mp, a);
  return res;
}

struct lvalue *builtin_mul(struct linterpreter *intp, struct lvalue *a) {
  LMATH_TYPE_CHECK(intp->lvalue_mp, a, "*");
  struct lvalue *res = lvalue_pop(intp->lvalue_mp, a, 0);

  if (res->type == LVAL_INT) {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      res->val.intval *= b->val.intval;
      lvalue_del(intp->lvalue_mp, b);
    }
  } else {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      res->val.floatval *= b->val.floatval;
      lvalue_del(intp->lvalue_mp, b);
    }
  }

  lvalue_del(intp->lvalue_mp, a);
  return res;
}

struct lvalue *builtin_div(struct linterpreter *intp, struct lvalue *a) {
  LMATH_TYPE_CHECK(intp->lvalue_mp, a, "/");
  struct lvalue *res = lvalue_pop(intp->lvalue_mp, a, 0);

  if (res->type == LVAL_INT) {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (b->val.intval == 0) {
        lvalue_del(intp->lvalue_mp, res);
        lvalue_del(intp->lvalue_mp, b);
        res = lvalue_err(intp->lvalue_mp, "Division by zero");
        break;
      }
      res->val.intval /= b->val.intval;
      lvalue_del(intp->lvalue_mp, b);
    }
  } else {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (b->val.floatval == 0) {
        lvalue_del(intp->lvalue_mp, res);
        lvalue_del(intp->lvalue_mp, b);
        res = lvalue_err(intp->lvalue_mp, "Division by zero");
        break;
      }
      res->val.floatval /= b->val.floatval;
      lvalue_del(intp->lvalue_mp, b);
    }
  }

  lvalue_del(intp->lvalue_mp, a);
  return res;
}

struct lvalue *builtin_mod(struct linterpreter *intp, struct lvalue *a) {
  LMATH_TYPE_CHECK(intp->lvalue_mp, a, "%");
  struct lvalue *res = lvalue_pop(intp->lvalue_mp, a, 0);

  if (res->type == LVAL_INT) {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (b->val.intval == 0) {
        lvalue_del(intp->lvalue_mp, res);
        lvalue_del(intp->lvalue_mp, b);
        res = lvalue_err(intp->lvalue_mp, "Division by zero");
        break;
      }
      res->val.intval %= b->val.intval;
      lvalue_del(intp->lvalue_mp, b);
    }
  } else {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (b->val.floatval == 0) {
        lvalue_del(intp->lvalue_mp, res);
        lvalue_del(intp->lvalue_mp, b);
        res = lvalue_err(intp->lvalue_mp, "Division by zero");
        break;
      }
      res->val.floatval = fmod(res->val.floatval, b->val.floatval);
      lvalue_del(intp->lvalue_mp, b);
    }
  }

  lvalue_del(intp->lvalue_mp, a);
  return res;
}

struct lvalue *builtin_min(struct linterpreter *intp, struct lvalue *a) {
  LMATH_TYPE_CHECK(intp->lvalue_mp, a, "min");
  struct lvalue *res = lvalue_pop(intp->lvalue_mp, a, 0);

  if (res->type == LVAL_INT) {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (res->val.intval > b->val.intval) {
        res->val.intval = b->val.intval;
      }
      lvalue_del(intp->lvalue_mp, b);
    }
  } else {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (res->val.floatval > b->val.floatval) {
        res->val.floatval = b->val.floatval;
      }
      lvalue_del(intp->lvalue_mp, b);
    }
  }

  lvalue_del(intp->lvalue_mp, a);
  return res;
}

struct lvalue *builtin_max(struct linterpreter *intp, struct lvalue *a) {
  LMATH_TYPE_CHECK(intp->lvalue_mp, a, "max");
  struct lvalue *res = lvalue_pop(intp->lvalue_mp, a, 0);

  if (res->type == LVAL_INT) {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (res->val.intval < b->val.intval) {
        res->val.intval = b->val.intval;
      }
      lvalue_del(intp->lvalue_mp, b);
    }
  } else {
    while (LCELLCOUNT(a) > 0) {
      struct lvalue *b = lvalue_pop(intp->lvalue_mp, a, 0);
      if (res->val.floatval < b->val.floatval) {
        res->val.floatval = b->val.floatval;
      }
      lvalue_del(intp->lvalue_mp, b);
    }
  }

  lvalue_del(intp->lvalue_mp, a);
  return res;
}

/* * q-expression specific builtins * */

/**
 * Convert expression into a q-expression
 */
struct lvalue *builtin_list(struct linterpreter *intp, struct lvalue *v) {
  UNUSED(intp->lvalue_mp);
  v->type = LVAL_QEXPR;
  return v;
}

/**
 * Take one q-expression and evaluate it
 * as a s-expression.
 */
struct lvalue *builtin_eval(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "eval", 1);
  LARG_TYPE(intp->lvalue_mp, v, "eval", 0, LVAL_QEXPR);

  struct lvalue *a = lvalue_take(intp->lvalue_mp, v, 0);
  a->type = LVAL_SEXPR;
  return lvalue_eval(intp, a);
}

/**
 * Read a string and try and parse into a
 * lvalue
 */
struct lvalue *builtin_read(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "read", 1);
  LARG_TYPE(intp->lvalue_mp, v, "read", 0, LVAL_STR);

  mpc_result_t r;
  if (mpc_parse("input", LGETCELL(v, 0)->val.strval, intp->grammar->Lisper, &r)) {
    struct lvalue *expr = lvalue_read(intp->lvalue_mp, r.output);
    mpc_ast_delete(r.output);

    lvalue_del(intp->lvalue_mp, v);
    return builtin_list(intp, expr);
  }

  /* parse error */
  char *err_msg = mpc_err_string(r.error);
  mpc_err_delete(r.error);

  struct lvalue *err = lvalue_err(intp->lvalue_mp, "Could parse str %s", err_msg);
  free(err_msg);
  lvalue_del(intp->lvalue_mp, v);

  return err;
}

/**
 * Print the contents of a string
 */
struct lvalue *builtin_show(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "show", 1);
  LARG_TYPE(intp->lvalue_mp, v, "show", 0, LVAL_STR);

  puts(LGETCELL(v, 0)->val.strval);

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_sexpr(intp->lvalue_mp);
}

/* * IO builtins * */

/**
 * get a list of input program arguments
 */
struct lvalue *builtin_args(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "args", 1);

  struct lvalue *res = lvalue_qexpr(intp->lvalue_mp);
  struct lvalue *arg;
  int argc = intp->argc;
  for (int i = 0; i < argc; ++i) {
    char *arg_str = intp->argv[i];
    arg = lvalue_str(intp->lvalue_mp, arg_str);
    lvalue_add(intp->lvalue_mp, res, arg);
  }

  lvalue_del(intp->lvalue_mp, v);
  return res;
}

/**
 * Print a series of lvalues
 */
struct lvalue *builtin_print(struct linterpreter *intp, struct lvalue *v) {

  for (size_t i = 0; i < LCELLCOUNT(v); ++i) {
    lvalue_print(LGETCELL(v, i));
    putchar(' ');
  }

  putchar('\n');
  lvalue_del(intp->lvalue_mp, v);

  return lvalue_sexpr(intp->lvalue_mp);
}

/**
 * Open a file in one of the following modes:
 * - "r": read-only mode
 * - "r+": read and write mode (on pre-existing file)
 * - "w": write-only mode that overrides existing files
 * - "a": append mode
 * - "w+": read and write mode that overrides existing files
 * - "a+": read and write mode that appends to existing files
 */
struct lvalue *builtin_open(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "open", 2);
  LARG_TYPE(intp->lvalue_mp, v, "open", 0, LVAL_STR);
  LARG_TYPE(intp->lvalue_mp, v, "open", 1, LVAL_STR);

  struct lvalue *filename = lvalue_pop(intp->lvalue_mp, v, 0);
  struct lvalue *mode = lvalue_pop(intp->lvalue_mp, v, 0);

  char *m = mode->val.strval;
  char *path = filename->val.strval;
  FILE *fp = NULL;

  if (!(strcmp(m, "r") == 0 || strcmp(m, "r+") == 0 || strcmp(m, "w") == 0 ||
        strcmp(m, "w+") == 0 || strcmp(m, "a") == 0 || strcmp(m, "a+") == 0)) {
    struct lvalue *err = lvalue_err(
        intp->lvalue_mp, "Mode not set to either 'r', 'r+', 'w', 'w+', 'a' or 'a+'");
    lvalue_del(intp->lvalue_mp, mode);
    lvalue_del(intp->lvalue_mp, filename);
    lvalue_del(intp->lvalue_mp, v);
    return err;
  }

  if (strlen(path) == 0) {
    lvalue_del(intp->lvalue_mp, mode);
    lvalue_del(intp->lvalue_mp, filename);
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp, "Empty file path");
  }

  fp = fopen(path, m);

  if (fp == NULL) {
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp, "Could not open file '%s'. %s", path,
                      strerror(errno));
  }

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_file(intp->lvalue_mp, filename, mode, fp);
}

/**
 * Closes an open file
 */
struct lvalue *builtin_close(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "close", 1);
  LARG_TYPE(intp->lvalue_mp, v, "close", 0, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 0);

  if (fclose(f->val.file->fp) != 0) {
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp, "Cloud not close file: '%s'", f->val.file->path);
  }

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_sexpr(intp->lvalue_mp);
}

struct lvalue *builtin_flush(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "flush", 1);
  LARG_TYPE(intp->lvalue_mp, v, "flush", 0, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 0);

  if (fflush(f->val.file->fp) != 0) {
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp, "Cloud not flush file buffer for: '%s'",
                      f->val.file->path);
  }

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_sexpr(intp->lvalue_mp);
}

struct lvalue *builtin_putstr(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "putstr", 2);
  LARG_TYPE(intp->lvalue_mp, v, "putstr", 0, LVAL_STR);
  LARG_TYPE(intp->lvalue_mp, v, "putstr", 1, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 1);
  struct lvalue *str = LGETCELL(v, 0);

  if (fputs(str->val.strval, f->val.file->fp) == EOF) {
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp, "Could write '%s' to file", str->val.strval);
  }

  return lvalue_sexpr(intp->lvalue_mp);
}

struct lvalue *builtin_getstr(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "getstr", 1);
  LARG_TYPE(intp->lvalue_mp, v, "getstr", 0, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 0);

  char *s = calloc(16385, sizeof(char));

  if (fgets(s, 16384 * sizeof(char), f->val.file->fp) == NULL) {
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp,
                      "Could not get string from file; could not read string");
  }

  char *resized = realloc(s, strlen(s) + 1);
  if (resized == NULL) {
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp,
                      "Could not get string from file; could not resize string buffer");
  }

  return lvalue_str(intp->lvalue_mp, resized);
}

struct lvalue *builtin_rewind(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "rewind", 1);
  LARG_TYPE(intp->lvalue_mp, v, "rewind", 0, LVAL_FILE);

  rewind(LGETCELL(v, 0)->val.file->fp);

  return lvalue_sexpr(intp->lvalue_mp);
}

/* * error builtins * */

struct lvalue *builtin_error(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "error", 1);
  LARG_TYPE(intp->lvalue_mp, v, "error", 0, LVAL_STR);

  struct lvalue *err = lvalue_err(intp->lvalue_mp, LGETCELL(v, 0)->val.strval);

  lvalue_del(intp->lvalue_mp, v);
  return err;
}

/* * collection builtins * */

struct lvalue *builtin_tail(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "tail", 1);
  LTWO_ARG_TYPES(intp->lvalue_mp, v, "tail", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *a = lvalue_take(intp->lvalue_mp, v, 0);

  if (a->type == LVAL_STR) {
    if (strlen(a->val.strval) > 1) {
      struct lvalue *tail = lvalue_str(intp->lvalue_mp, a->val.strval + 1);
      lvalue_del(intp->lvalue_mp, a);
      return tail;
    }
  } else {

    if (LCELLCOUNT(a) > 0) {
      lvalue_del(intp->lvalue_mp, lvalue_pop(intp->lvalue_mp, a, 0));
      return a;
    }
  }

  char *typename = ltype_name(a->type);
  lvalue_del(intp->lvalue_mp, a);
  return lvalue_err(intp->lvalue_mp, "Attempt to take the tail of empty %s", typename);
}

struct lvalue *builtin_head(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "head", 1);
  LTWO_ARG_TYPES(intp->lvalue_mp, v, "head", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *a = lvalue_take(intp->lvalue_mp, v, 0);

  if (a->type == LVAL_STR) {
    if (strlen(a->val.strval) > 1) {
      char second = a->val.strval[1];
      a->val.strval[1] = '\0';

      struct lvalue *head = lvalue_str(intp->lvalue_mp, a->val.strval);
      a->val.strval[1] = second;

      lvalue_del(intp->lvalue_mp, a);
      return head;
    }
    lvalue_del(intp->lvalue_mp, a);
    return lvalue_str(intp->lvalue_mp, "");
  } else {
    while (LCELLCOUNT(a) > 1) {
      lvalue_del(intp->lvalue_mp, lvalue_pop(intp->lvalue_mp, a, 1));
    }
    return a;
  }
}

struct lvalue *builtin_join(struct linterpreter *intp, struct lvalue *v) {
  for (size_t i = 0; i < LCELLCOUNT(v); ++i) {
    LTWO_ARG_TYPES(intp->lvalue_mp, v, "join", i, LVAL_QEXPR, LVAL_STR);
  }

  struct lvalue *a = lvalue_pop(intp->lvalue_mp, v, 0);

  if (a->type == LVAL_STR) {
    while (LCELLCOUNT(v)) {
      a = lvalue_join_str(intp->lvalue_mp, a, lvalue_pop(intp->lvalue_mp, v, 0));
    }
    lvalue_del(intp->lvalue_mp, v);
    return a;
  } else {
    while (LCELLCOUNT(v)) {
      a = lvalue_join(intp->lvalue_mp, a, lvalue_pop(intp->lvalue_mp, v, 0));
    }

    lvalue_del(intp->lvalue_mp, v);
    return a;
  }
}

struct lvalue *builtin_cons(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "cons", 2);
  LARG_TYPE(intp->lvalue_mp, v, "cons", 1, LVAL_QEXPR);

  struct lvalue *consvalue = lvalue_pop(intp->lvalue_mp, v, 0);
  struct lvalue *collection = lvalue_pop(intp->lvalue_mp, v, 0);

  lvalue_offer(intp->lvalue_mp, collection, consvalue);

  lvalue_del(intp->lvalue_mp, v);
  return collection;
}

struct lvalue *builtin_len(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "len", 1);
  LTWO_ARG_TYPES(intp->lvalue_mp, v, "len", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *arg = LGETCELL(v, 0);
  size_t count = 0;
  if (arg->type == LVAL_STR) {
    count = strlen(arg->val.strval);
  } else {
    count = LCELLCOUNT(arg);
  }

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_int(intp->lvalue_mp, count);
}

struct lvalue *builtin_init(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "init", 1);
  LTWO_ARG_TYPES(intp->lvalue_mp, v, "init", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *collection = lvalue_pop(intp->lvalue_mp, v, 0);

  if (collection->type == LVAL_QEXPR) {
    if (LCELLCOUNT(collection) > 0) {
      lvalue_del(intp->lvalue_mp,
                 lvalue_pop(intp->lvalue_mp, collection, (LCELLCOUNT(collection) - 1)));
    }
  } else {
    size_t origsize = strlen(collection->val.strval);
    collection->val.strval[origsize - 1] = '\0';
    char *resized = realloc(collection->val.strval, origsize);
    if (resized == NULL) {
      perror("Could not resize char array");
      lvalue_del(intp->lvalue_mp, v);
      lenvironment_del(intp->lvalue_mp, intp->env);
      exit(1);
    }
    collection->val.strval = resized;
  }

  lvalue_del(intp->lvalue_mp, v);
  return collection;
}

/* * control flow builtins  * */

struct lvalue *builtin_exit(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "exit", 1);
  LARG_TYPE(intp->lvalue_mp, v, "exit", 0, LVAL_INT);

  int exit_code = (int)(LGETCELL(v, 0)->val.intval);
  lvalue_del(intp->lvalue_mp, v);

  intp->halt_type = LINTERP_USER_EXIT;
  intp->halt_value.rc = exit_code;

  return lvalue_exit(intp->lvalue_mp, exit_code);
}

struct lvalue *builtin_if(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "if", 3);
  LARG_TYPE(intp->lvalue_mp, v, "if", 0, LVAL_BOOL);
  LARG_TYPE(intp->lvalue_mp, v, "if", 1, LVAL_QEXPR);
  LARG_TYPE(intp->lvalue_mp, v, "if", 2, LVAL_QEXPR);

  struct lvalue *res = NULL;
  struct lvalue *cond = LGETCELL(v, 0);

  if (cond->val.intval) {
    res = lvalue_pop(intp->lvalue_mp, v, 1);
    res->type = LVAL_SEXPR;
    res = lvalue_eval(intp, res);
  } else {
    res = lvalue_pop(intp->lvalue_mp, v, 2);
    res->type = LVAL_SEXPR;
    res = lvalue_eval(intp, res);
  }

  lvalue_del(intp->lvalue_mp, v);
  return res;
}

/* * reflection builtins * */

struct lvalue *builtin_type(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "type", 1);

  char *t = ltype_name(LGETCELL(v, 0)->type);

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_str(intp->lvalue_mp, t);
}

/* * function builtins * */

struct lvalue *builtin_lambda(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "\\", 2);
  LARG_TYPE(intp->lvalue_mp, v, "\\", 0, LVAL_QEXPR);
  LARG_TYPE(intp->lvalue_mp, v, "\\", 1, LVAL_QEXPR);

  struct lvalue *formals = LGETCELL(v, 0);
  struct lvalue *body = LGETCELL(v, 1);

  for (size_t i = 0; i < LCELLCOUNT(formals); ++i) {
    LASSERT(intp->lvalue_mp, v, LGETCELL(formals, i)->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", i + 1,
            ltype_name(LVAL_SYM), ltype_name(LGETCELL(formals, i)->type));
  }

  formals = lvalue_pop(intp->lvalue_mp, v, 0);
  body = lvalue_pop(intp->lvalue_mp, v, 0);
  lvalue_del(intp->lvalue_mp, v);

  return lvalue_lambda(intp->lvalue_mp, formals, body, lambda_env_prealloc);
}

/**
 *  Function declaration builtin
 */
struct lvalue *builtin_fn(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "fn", 3);
  LARG_TYPE(intp->lvalue_mp, v, "fn", 0, LVAL_QEXPR); // function name
  LARG_TYPE(intp->lvalue_mp, v, "fn", 1, LVAL_QEXPR); // parameter list
  LARG_TYPE(intp->lvalue_mp, v, "fn", 2, LVAL_QEXPR); // body

  struct lvalue *name = LGETCELL(v, 0);
  struct lvalue *formals = LGETCELL(v, 1);
  struct lvalue *body = LGETCELL(v, 2);

  /* checking function name */
  LASSERT(intp->lvalue_mp, v, LCELLCOUNT(name) == 1,
          "Function was parsed a empty function name. A function must be named", 0);
  LASSERT(intp->lvalue_mp, v, LGETCELL(name, 0)->type == LVAL_SYM,
          "Expected function name to be of type '%s'; got type '%s'.",
          ltype_name(LVAL_SYM), ltype_name(LGETCELL(name, 0)->type));

  /* checking formals parameters */
  for (size_t i = 0; i < LCELLCOUNT(formals); ++i) {
    LASSERT(intp->lvalue_mp, v, LGETCELL(formals, i)->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", i + 1,
            ltype_name(LVAL_SYM), ltype_name(LGETCELL(formals, i)->type));
  }

  /* params are ok poping them off the value */
  name = lvalue_pop(intp->lvalue_mp, v, 0);
  formals = lvalue_pop(intp->lvalue_mp, v, 0);
  body = lvalue_pop(intp->lvalue_mp, v, 0);

  struct lvalue *fn = lvalue_lambda(intp->lvalue_mp, formals, body, fun_env_prealloc);

  lenvironment_put(intp->lvalue_mp, intp->env, LGETCELL(name, 0), fn);
  lvalue_del(intp->lvalue_mp, fn);
  lvalue_del(intp->lvalue_mp, name);
  lvalue_del(intp->lvalue_mp, v);

  return lvalue_sexpr(intp->lvalue_mp);
}

/* * value definition builtins * */

struct lvalue *builtin_var(struct linterpreter *intp, struct lvalue *, char *);

struct lvalue *builtin_def(struct linterpreter *intp, struct lvalue *v) {
  return builtin_var(intp, v, "def");
}

struct lvalue *builtin_put(struct linterpreter *intp, struct lvalue *v) {
  return builtin_var(intp, v, "=");
}

struct lvalue *builtin_var(struct linterpreter *intp, struct lvalue *v, char *sym) {
  LARG_TYPE(intp->lvalue_mp, v, sym, 0, LVAL_QEXPR);

  struct lvalue *names = LGETCELL(v, 0);

  for (size_t i = 0; i < LCELLCOUNT(names); ++i) {
    LASSERT(intp->lvalue_mp, v, LGETCELL(names, i)->type == LVAL_SYM,
            "Function '%s' cannot assign value(s) to name(s). Name %lu is of type '%s'; "
            "expected type '%s'.",
            sym, i, ltype_name(LGETCELL(names, i)->type), ltype_name(LVAL_SYM));
  }

  LASSERT(intp->lvalue_mp, v, LCELLCOUNT(names) == LCELLCOUNT(v) - 1,
          "Function '%s' cannot assign value(s) to name(s). Number of name(s) and "
          "value(s) does not match. Saw %lu name(s) expected %lu value(s).",
          sym, LCELLCOUNT(names), LCELLCOUNT(v) - 1);

  for (size_t i = 0; i < LCELLCOUNT(names); ++i) {
    if (strcmp(sym, "def") == 0) {
      lenvironment_def(intp->lvalue_mp, intp->env, LGETCELL(names, i),
                       LGETCELL(v, i + 1));
    } else if (strcmp(sym, "=") == 0) {
      lenvironment_put(intp->lvalue_mp, intp->env, LGETCELL(names, i),
                       LGETCELL(v, i + 1));
    }
  }

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_sexpr(intp->lvalue_mp);
}

/* * comparison builtins * */

struct lvalue *builtin_ord(struct linterpreter *intp, struct lvalue *v, char *sym);

struct lvalue *builtin_lt(struct linterpreter *intp, struct lvalue *v) {
  return builtin_ord(intp, v, "<");
}

struct lvalue *builtin_gt(struct linterpreter *intp, struct lvalue *v) {
  return builtin_ord(intp, v, ">");
}

struct lvalue *builtin_le(struct linterpreter *intp, struct lvalue *v) {
  return builtin_ord(intp, v, "<=");
}

struct lvalue *builtin_ge(struct linterpreter *intp, struct lvalue *v) {
  return builtin_ord(intp, v, ">=");
}

struct lvalue *builtin_ord(struct linterpreter *intp, struct lvalue *v, char *sym) {
  LNUM_ARGS(intp->lvalue_mp, v, sym, 2);

  struct lvalue *lhs = LGETCELL(v, 0);
  struct lvalue *rhs = LGETCELL(v, 1);

  LASSERT(intp->lvalue_mp, v, LIS_NUM(lhs->type),
          "Wrong type of argument parsed to '%s'. Expected '%s' or '%s' got '%s'.", sym,
          ltype_name(LVAL_INT), ltype_name(LVAL_FLOAT), ltype_name(lhs->type));
  LASSERT(intp->lvalue_mp, v, LIS_NUM(rhs->type),
          "Wrong type of argument parsed to '%s'. Expected '%s' or '%s' got '%s'.", sym,
          ltype_name(LVAL_INT), ltype_name(LVAL_FLOAT), ltype_name(rhs->type));
  LASSERT(intp->lvalue_mp, v, lhs->type == rhs->type,
          "Type of arguments does not match. Argument %i is of type '%s'; argument %i is "
          "of type '%s'.",
          1, ltype_name(lhs->type), 2, ltype_name(rhs->type));

  long long res = 0;
  if (strcmp(sym, "<") == 0) {
    res = (lhs->val.intval < rhs->val.intval);
  } else if (strcmp(sym, ">") == 0) {
    res = (lhs->val.intval > rhs->val.intval);
  } else if (strcmp(sym, "<=") == 0) {
    res = (lhs->val.intval <= rhs->val.intval);
  } else if (strcmp(sym, ">=") == 0) {
    res = (lhs->val.intval >= rhs->val.intval);
  }

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_bool(intp->lvalue_mp, res);
}

struct lvalue *builtin_cmp(struct linterpreter *intp, struct lvalue *v, char *sym) {
  LNUM_ARGS(intp->lvalue_mp, v, sym, 2);

  struct lvalue *lhs = LGETCELL(v, 0);
  struct lvalue *rhs = LGETCELL(v, 1);

  long long res = 0;
  if (strcmp(sym, "==") == 0) {
    res = lvalue_eq(lhs, rhs);
  } else if (strcmp(sym, "!=") == 0) {
    res = !lvalue_eq(lhs, rhs);
  }

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_bool(intp->lvalue_mp, res);
}

struct lvalue *builtin_eq(struct linterpreter *intp, struct lvalue *v) {
  return builtin_cmp(intp, v, "==");
}

struct lvalue *builtin_ne(struct linterpreter *intp, struct lvalue *v) {
  return builtin_cmp(intp, v, "!=");
}

/* logical builtins */

struct lvalue *builtin_and(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "&&", 2);
  LARG_TYPE(intp->lvalue_mp, v, "&&", 0, LVAL_BOOL);
  LARG_TYPE(intp->lvalue_mp, v, "&&", 1, LVAL_BOOL);

  long long res = (LGETCELL(v, 0)->val.intval && LGETCELL(v, 1)->val.intval);

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_bool(intp->lvalue_mp, res);
}

struct lvalue *builtin_or(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "||", 2);
  LARG_TYPE(intp->lvalue_mp, v, "||", 0, LVAL_BOOL);
  LARG_TYPE(intp->lvalue_mp, v, "||", 1, LVAL_BOOL);

  long long res = (LGETCELL(v, 0)->val.intval || LGETCELL(v, 1)->val.intval);

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_bool(intp->lvalue_mp, res);
}

struct lvalue *builtin_not(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "!", 1);
  LARG_TYPE(intp->lvalue_mp, v, "!", 0, LVAL_BOOL);

  long long res = !LGETCELL(v, 0)->val.intval;

  lvalue_del(intp->lvalue_mp, v);
  return lvalue_bool(intp->lvalue_mp, res);
}

/* source importation builtins */

struct lvalue *builtin_load(struct linterpreter *intp, struct lvalue *v) {
  LNUM_ARGS(intp->lvalue_mp, v, "load", 1);
  LARG_TYPE(intp->lvalue_mp, v, "load", 0, LVAL_STR);

  mpc_result_t r;
  if (mpc_parse_contents(LGETCELL(v, 0)->val.strval, intp->grammar->Lisper, &r)) {

    struct lvalue *expr = lvalue_read(intp->lvalue_mp, r.output);
    mpc_ast_delete(r.output);

    while (LCELLCOUNT(expr)) {
      struct lvalue *x = lvalue_eval(intp, lvalue_pop(intp->lvalue_mp, expr, 0));
      if (x->type == LVAL_USER_EXIT) {
        lvalue_del(intp->lvalue_mp, expr);
        return x;
      }

      if (x->type == LVAL_ERR) {
        lvalue_println(x);
      }
      lvalue_del(intp->lvalue_mp, x);
    }

    lvalue_del(intp->lvalue_mp, expr);
    lvalue_del(intp->lvalue_mp, v);

    return lvalue_sexpr(intp->lvalue_mp);
  } else {
    /* parse error */
    char *err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);

    struct lvalue *err =
        lvalue_err(intp->lvalue_mp, "Could not load library %s", err_msg);
    free(err_msg);
    lvalue_del(intp->lvalue_mp, v);

    return err;
  }
}

void register_builtins(struct linterpreter *intp) {
  lenvironment_add_builtin(intp, "list", builtin_list);
  lenvironment_add_builtin(intp, "head", builtin_head);
  lenvironment_add_builtin(intp, "tail", builtin_tail);
  lenvironment_add_builtin(intp, "eval", builtin_eval);
  lenvironment_add_builtin(intp, "join", builtin_join);
  lenvironment_add_builtin(intp, "cons", builtin_cons);
  lenvironment_add_builtin(intp, "len", builtin_len);
  lenvironment_add_builtin(intp, "init", builtin_init);
  lenvironment_add_builtin(intp, "def", builtin_def);
  lenvironment_add_builtin(intp, "exit", builtin_exit);
  lenvironment_add_builtin(intp, "max", builtin_max);
  lenvironment_add_builtin(intp, "min", builtin_min);
  lenvironment_add_builtin(intp, "fn", builtin_fn);
  lenvironment_add_builtin(intp, "if", builtin_if);
  lenvironment_add_builtin(intp, "args", builtin_args);
  lenvironment_add_builtin(intp, "type", builtin_type);
  lenvironment_add_builtin(intp, "load", builtin_load);
  lenvironment_add_builtin(intp, "error", builtin_error);
  lenvironment_add_builtin(intp, "print", builtin_print);
  lenvironment_add_builtin(intp, "read", builtin_read);
  lenvironment_add_builtin(intp, "show", builtin_show);
  lenvironment_add_builtin(intp, "open", builtin_open);
  lenvironment_add_builtin(intp, "close", builtin_close);
  lenvironment_add_builtin(intp, "flush", builtin_flush);
  lenvironment_add_builtin(intp, "putstr", builtin_putstr);
  lenvironment_add_builtin(intp, "rewind", builtin_rewind);
  lenvironment_add_builtin(intp, "getstr", builtin_getstr);

  lenvironment_add_builtin(intp, "+", builtin_add);
  lenvironment_add_builtin(intp, "-", builtin_sub);
  lenvironment_add_builtin(intp, "*", builtin_mul);
  lenvironment_add_builtin(intp, "/", builtin_div);
  lenvironment_add_builtin(intp, "%", builtin_mod);
  lenvironment_add_builtin(intp, "\\", builtin_lambda);
  lenvironment_add_builtin(intp, "=", builtin_put);

  lenvironment_add_builtin(intp, "==", builtin_eq);
  lenvironment_add_builtin(intp, "!=", builtin_ne);

  lenvironment_add_builtin(intp, "||", builtin_or);
  lenvironment_add_builtin(intp, "&&", builtin_and);
  lenvironment_add_builtin(intp, "!", builtin_not);

  lenvironment_add_builtin(intp, ">", builtin_gt);
  lenvironment_add_builtin(intp, "<", builtin_lt);
  lenvironment_add_builtin(intp, ">=", builtin_ge);
  lenvironment_add_builtin(intp, "<=", builtin_le);
}
