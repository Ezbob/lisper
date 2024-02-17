#include "builtin.h"
#include "environment.h"
#include "grammar.h"
#include "lisper.h"
#include "value.h"
#include "value/constructors.h"
#include "value/lfile.h"
#include "value/lvalue.h"
#include "value/transformers.h"
#include <math.h>
#include <stdlib.h>

#define LGETCELL(v, celln) v->val.l.cells[celln]

#define UNUSED(x) (void)(x)

#define LIS_NUM(type) (type == LVAL_INT || type == LVAL_FLOAT)

#define LASSERT(lval, cond, fmt, ...)                                                    \
  if (!(cond)) {                                                                         \
    struct lvalue *err = lvalue_err(fmt, ##__VA_ARGS__);                                 \
    lvalue_del(lval);                                                                    \
    return err;                                                                          \
  }

#define LNUM_LEAST_ARGS(sym, funcname, numargs)                                          \
  LASSERT(sym, sym->val.l.count >= numargs,                                              \
          "Wrong number of arguments parsed to '%s'. Expected at least %lu "             \
          "argument(s); got %lu. ",                                                      \
          funcname, numargs, sym->val.l.count)

#define LNUM_ARGS(lval, func_name, numargs)                                              \
  LASSERT(lval, lval->val.l.count == numargs,                                            \
          "Wrong number of arguments parsed to '%s'. Expected at exactly %lu "           \
          "argument(s); got %lu. ",                                                      \
          func_name, numargs, lval->val.l.count)

#define LNOT_EMPTY_QEXPR(lval, func_name, i)                                             \
  LASSERT(lval,                                                                          \
          lval->val.l.cells[i]->type == LVAL_QEXPR &&                                    \
              lval->val.l.cells[i]->val.l.count > 0,                                     \
          "Empty %s parsed to '%s'.", ltype_name(lval->val.l.cells[i]->type, func_name))

#define LARG_TYPE(lval, func_name, i, expected)                                          \
  LASSERT(lval, lval->val.l.cells[i]->type == expected,                                  \
          "Wrong type of argument parsed to '%s' at argument position %lu. Expected "    \
          "argument to be of type '%s'; got '%s'.",                                      \
          func_name, (i + 1), ltype_name(expected),                                      \
          ltype_name(lval->val.l.cells[i]->type))

#define LTWO_ARG_TYPES(lval, func_name, i, first_expect, second_expect)                  \
  LASSERT(lval,                                                                          \
          lval->val.l.cells[i]->type == first_expect ||                                  \
              lval->val.l.cells[i]->type == second_expect,                               \
          "Wrong type of argument parsed to '%s'. Expected argument to be of type '%s' " \
          "or '%s'; got '%s'.",                                                          \
          func_name, ltype_name(first_expect), ltype_name(second_expect),                \
          ltype_name(lval->val.l.cells[i]->type))

#define LENV_BUILTIN(name) lenvironment_add_builtin(e, #name, builtin_##name)
#define LENV_SYMBUILTIN(sym, name) lenvironment_add_builtin(e, sym, builtin_##name)

#define LMATH_TYPE_CHECK(lval, sym)                                                      \
  do {                                                                                   \
    enum ltype expected_arg_type = LGETCELL(lval, 0)->type;                              \
    LASSERT(lval, LIS_NUM(expected_arg_type),                                            \
            "Cannot operate on argument at position %i. Non-number type '%s' parsed to " \
            "operator '%s'.",                                                            \
            1, ltype_name(expected_arg_type), sym);                                      \
    for (size_t i = 1; i < lval->val.l.count; i++) {                                     \
      struct lvalue *curr = LGETCELL(lval, i);                                           \
      LASSERT(lval, expected_arg_type == curr->type,                                     \
              "Argument type mismatch. Expected argument at position %lu to be of type " \
              "'%s'; got type '%s'.",                                                    \
              i + 1, ltype_name(expected_arg_type), ltype_name(curr->type));             \
    }                                                                                    \
  } while (0)

extern struct grammar_elems elems;
extern struct argument_capture *args;

/* env preallocated sizes */
const size_t lambda_env_prealloc = 50;
const size_t fun_env_prealloc = 200;

/* * math builtins * */

struct lvalue *builtin_add(struct lenvironment *e, struct lvalue *in) {
  UNUSED(e);
  LMATH_TYPE_CHECK(in, "+");
  struct lvalue *res = lvalue_pop(in, 0);

  if (res->type == LVAL_INT) {
    while (in->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(in, 0);
      res->val.intval += b->val.intval;
      lvalue_del(b);
    }
  } else {
    while (in->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(in, 0);
      res->val.floatval += b->val.floatval;
      lvalue_del(b);
    }
  }

  lvalue_del(in);
  return res;
}

struct lvalue *builtin_sub(struct lenvironment *e, struct lvalue *a) {
  UNUSED(e);
  LMATH_TYPE_CHECK(a, "-");
  struct lvalue *res = lvalue_pop(a, 0);

  if (res->type == LVAL_INT) {
    if (a->val.l.count == 0) {
      res->val.intval = (-res->val.intval);
    } else {
      while (a->val.l.count > 0) {
        struct lvalue *b = lvalue_pop(a, 0);
        res->val.intval -= b->val.intval;
        lvalue_del(b);
      }
    }
  } else {
    if (a->val.l.count == 0) {
      res->val.floatval = (-res->val.floatval);
    } else {
      while (a->val.l.count > 0) {
        struct lvalue *b = lvalue_pop(a, 0);
        res->val.floatval -= b->val.floatval;
        lvalue_del(b);
      }
    }
  }

  lvalue_del(a);
  return res;
}

struct lvalue *builtin_mul(struct lenvironment *e, struct lvalue *a) {
  UNUSED(e);
  LMATH_TYPE_CHECK(a, "*");
  struct lvalue *res = lvalue_pop(a, 0);

  if (res->type == LVAL_INT) {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      res->val.intval *= b->val.intval;
      lvalue_del(b);
    }
  } else {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      res->val.floatval *= b->val.floatval;
      lvalue_del(b);
    }
  }

  lvalue_del(a);
  return res;
}

struct lvalue *builtin_div(struct lenvironment *e, struct lvalue *a) {
  UNUSED(e);
  LMATH_TYPE_CHECK(a, "/");
  struct lvalue *res = lvalue_pop(a, 0);

  if (res->type == LVAL_INT) {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (b->val.intval == 0) {
        lvalue_del(res);
        lvalue_del(b);
        res = lvalue_err("Division by zero");
        break;
      }
      res->val.intval /= b->val.intval;
      lvalue_del(b);
    }
  } else {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (b->val.floatval == 0) {
        lvalue_del(res);
        lvalue_del(b);
        res = lvalue_err("Division by zero");
        break;
      }
      res->val.floatval /= b->val.floatval;
      lvalue_del(b);
    }
  }

  lvalue_del(a);
  return res;
}

struct lvalue *builtin_mod(struct lenvironment *e, struct lvalue *a) {
  UNUSED(e);
  LMATH_TYPE_CHECK(a, "%");
  struct lvalue *res = lvalue_pop(a, 0);

  if (res->type == LVAL_INT) {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (b->val.intval == 0) {
        lvalue_del(res);
        lvalue_del(b);
        res = lvalue_err("Division by zero");
        break;
      }
      res->val.intval %= b->val.intval;
      lvalue_del(b);
    }
  } else {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (b->val.floatval == 0) {
        lvalue_del(res);
        lvalue_del(b);
        res = lvalue_err("Division by zero");
        break;
      }
      res->val.floatval = fmod(res->val.floatval, b->val.floatval);
      lvalue_del(b);
    }
  }

  lvalue_del(a);
  return res;
}

struct lvalue *builtin_min(struct lenvironment *e, struct lvalue *a) {
  UNUSED(e);
  LMATH_TYPE_CHECK(a, "min");
  struct lvalue *res = lvalue_pop(a, 0);

  if (res->type == LVAL_INT) {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (res->val.intval > b->val.intval) {
        res->val.intval = b->val.intval;
      }
      lvalue_del(b);
    }
  } else {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (res->val.floatval > b->val.floatval) {
        res->val.floatval = b->val.floatval;
      }
      lvalue_del(b);
    }
  }

  lvalue_del(a);
  return res;
}

struct lvalue *builtin_max(struct lenvironment *e, struct lvalue *a) {
  UNUSED(e);
  LMATH_TYPE_CHECK(a, "max");
  struct lvalue *res = lvalue_pop(a, 0);

  if (res->type == LVAL_INT) {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (res->val.intval < b->val.intval) {
        res->val.intval = b->val.intval;
      }
      lvalue_del(b);
    }
  } else {
    while (a->val.l.count > 0) {
      struct lvalue *b = lvalue_pop(a, 0);
      if (res->val.floatval < b->val.floatval) {
        res->val.floatval = b->val.floatval;
      }
      lvalue_del(b);
    }
  }

  lvalue_del(a);
  return res;
}

/* * q-expression specific builtins * */

/**
 * Convert expression into a q-expression
 */
struct lvalue *builtin_list(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  v->type = LVAL_QEXPR;
  return v;
}

/**
 * Take one q-expression and evaluate it
 * as a s-expression.
 */
struct lvalue *builtin_eval(struct lenvironment *e, struct lvalue *v) {
  LNUM_ARGS(v, "eval", 1);
  LARG_TYPE(v, "eval", 0, LVAL_QEXPR);

  struct lvalue *a = lvalue_take(v, 0);
  a->type = LVAL_SEXPR;
  return lvalue_eval(e, a);
}

/**
 * Read a string and try and parse into a
 * lvalue
 */
struct lvalue *builtin_read(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "read", 1);
  LARG_TYPE(v, "read", 0, LVAL_STR);

  mpc_result_t r;
  if (mpc_parse("input", LGETCELL(v, 0)->val.strval, elems.Lisper, &r)) {
    struct lvalue *expr = lvalue_read(r.output);
    mpc_ast_delete(r.output);

    lvalue_del(v);
    return builtin_list(e, expr);
  }

  /* parse error */
  char *err_msg = mpc_err_string(r.error);
  mpc_err_delete(r.error);

  struct lvalue *err = lvalue_err("Could parse str %s", err_msg);
  free(err_msg);
  lvalue_del(v);

  return err;
}

/**
 * Print the contents of a string
 */
struct lvalue *builtin_show(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "show", 1);
  LARG_TYPE(v, "show", 0, LVAL_STR);

  puts(LGETCELL(v, 0)->val.strval);

  lvalue_del(v);
  return lvalue_sexpr();
}

/* * IO builtins * */

/**
 * get a list of input program arguments
 */
struct lvalue *builtin_args(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "args", 1);

  struct lvalue *res = lvalue_qexpr();
  struct lvalue *arg;
  int argc = args->argc;
  for (int i = 0; i < argc; ++i) {
    char *arg_str = args->argv[i];
    arg = lvalue_str(arg_str);
    lvalue_add(res, arg);
  }

  lvalue_del(v);
  return res;
}

/**
 * Print a series of lvalues
 */
struct lvalue *builtin_print(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);

  for (size_t i = 0; i < v->val.l.count; ++i) {
    lvalue_print(LGETCELL(v, i));
    putchar(' ');
  }

  putchar('\n');
  lvalue_del(v);

  return lvalue_sexpr();
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
struct lvalue *builtin_open(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "open", 2);
  LARG_TYPE(v, "open", 0, LVAL_STR);
  LARG_TYPE(v, "open", 1, LVAL_STR);

  struct lvalue *filename = lvalue_pop(v, 0);
  struct lvalue *mode = lvalue_pop(v, 0);

  char *m = mode->val.strval;
  char *path = filename->val.strval;
  FILE *fp = NULL;

  if (!(strcmp(m, "r") == 0 || strcmp(m, "r+") == 0 || strcmp(m, "w") == 0 ||
        strcmp(m, "w+") == 0 || strcmp(m, "a") == 0 || strcmp(m, "a+") == 0)) {
    struct lvalue *err =
        lvalue_err("Mode not set to either 'r', 'r+', 'w', 'w+', 'a' or 'a+'");
    lvalue_del(mode);
    lvalue_del(filename);
    lvalue_del(v);
    return err;
  }

  if (strlen(path) == 0) {
    lvalue_del(mode);
    lvalue_del(filename);
    lvalue_del(v);
    return lvalue_err("Empty file path");
  }

  fp = fopen(path, m);

  if (fp == NULL) {
    lvalue_del(v);
    return lvalue_err("Could not open file '%s'. %s", path, strerror(errno));
  }

  lvalue_del(v);
  return lvalue_file(filename, mode, fp);
}

/**
 * Closes an open file
 */
struct lvalue *builtin_close(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "close", 1);
  LARG_TYPE(v, "close", 0, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 0);

  if (fclose(f->val.file->fp) != 0) {
    lvalue_del(v);
    return lvalue_err("Cloud not close file: '%s'", f->val.file->path);
  }

  lvalue_del(v);
  return lvalue_sexpr();
}

struct lvalue *builtin_flush(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "flush", 1);
  LARG_TYPE(v, "flush", 0, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 0);

  if (fflush(f->val.file->fp) != 0) {
    lvalue_del(v);
    return lvalue_err("Cloud not flush file buffer for: '%s'", f->val.file->path);
  }

  lvalue_del(v);
  return lvalue_sexpr();
}

struct lvalue *builtin_putstr(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "putstr", 2);
  LARG_TYPE(v, "putstr", 0, LVAL_STR);
  LARG_TYPE(v, "putstr", 1, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 1);
  struct lvalue *str = LGETCELL(v, 0);

  if (fputs(str->val.strval, f->val.file->fp) == EOF) {
    lvalue_del(v);
    return lvalue_err("Could write '%s' to file", str->val.strval);
  }

  return lvalue_sexpr();
}

struct lvalue *builtin_getstr(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "getstr", 1);
  LARG_TYPE(v, "getstr", 0, LVAL_FILE);

  struct lvalue *f = LGETCELL(v, 0);

  char *s = calloc(16385, sizeof(char));

  if (fgets(s, 16384 * sizeof(char), f->val.file->fp) == NULL) {
    lvalue_del(v);
    return lvalue_err("Could not get string from file; could not read string");
  }

  char *resized = realloc(s, strlen(s) + 1);
  if (resized == NULL) {
    lvalue_del(v);
    return lvalue_err("Could not get string from file; could not resize string buffer");
  }

  return lvalue_str(resized);
}

struct lvalue *builtin_rewind(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "rewind", 1);
  LARG_TYPE(v, "rewind", 0, LVAL_FILE);

  rewind(LGETCELL(v, 0)->val.file->fp);

  return lvalue_sexpr();
}

/* * error builtins * */

struct lvalue *builtin_error(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "error", 1);
  LARG_TYPE(v, "error", 0, LVAL_STR);

  struct lvalue *err = lvalue_err(LGETCELL(v, 0)->val.strval);

  lvalue_del(v);
  return err;
}

/* * collection builtins * */

struct lvalue *builtin_tail(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "tail", 1);
  LTWO_ARG_TYPES(v, "tail", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *a = lvalue_take(v, 0);

  if (a->type == LVAL_STR) {
    if (strlen(a->val.strval) > 1) {
      struct lvalue *tail = lvalue_str(a->val.strval + 1);
      lvalue_del(a);
      return tail;
    }
  } else {

    if (a->val.l.count > 0) {
      lvalue_del(lvalue_pop(a, 0));
      return a;
    }
  }

  char *typename = ltype_name(a->type);
  lvalue_del(a);
  return lvalue_err("Attempt to take the tail of empty %s", typename);
}

struct lvalue *builtin_head(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "head", 1);
  LTWO_ARG_TYPES(v, "head", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *a = lvalue_take(v, 0);

  if (a->type == LVAL_STR) {
    if (strlen(a->val.strval) > 1) {
      char second = a->val.strval[1];
      a->val.strval[1] = '\0';

      struct lvalue *head = lvalue_str(a->val.strval);
      a->val.strval[1] = second;

      lvalue_del(a);
      return head;
    }
    lvalue_del(a);
    return lvalue_str("");
  } else {
    while (a->val.l.count > 1) {
      lvalue_del(lvalue_pop(a, 1));
    }
    return a;
  }
}

struct lvalue *builtin_join(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  for (size_t i = 0; i < v->val.l.count; ++i) {
    LTWO_ARG_TYPES(v, "join", i, LVAL_QEXPR, LVAL_STR);
  }

  struct lvalue *a = lvalue_pop(v, 0);

  if (a->type == LVAL_STR) {
    while (v->val.l.count) {
      a = lvalue_join_str(a, lvalue_pop(v, 0));
    }
    lvalue_del(v);
    return a;
  } else {
    while (v->val.l.count) {
      a = lvalue_join(a, lvalue_pop(v, 0));
    }

    lvalue_del(v);
    return a;
  }
}

struct lvalue *builtin_cons(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "cons", 2);
  LARG_TYPE(v, "cons", 1, LVAL_QEXPR);

  struct lvalue *consvalue = lvalue_pop(v, 0);
  struct lvalue *collection = lvalue_pop(v, 0);

  lvalue_offer(collection, consvalue);

  lvalue_del(v);
  return collection;
}

struct lvalue *builtin_len(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "len", 1);
  LTWO_ARG_TYPES(v, "len", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *arg = LGETCELL(v, 0);
  size_t count = 0;
  if (arg->type == LVAL_STR) {
    count = strlen(arg->val.strval);
  } else {
    count = arg->val.l.count;
  }

  lvalue_del(v);
  return lvalue_int(count);
}

struct lvalue *builtin_init(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "init", 1);
  LTWO_ARG_TYPES(v, "init", 0, LVAL_QEXPR, LVAL_STR);

  struct lvalue *collection = lvalue_pop(v, 0);

  if (collection->type == LVAL_QEXPR) {
    if (collection->val.l.count > 0) {
      lvalue_del(lvalue_pop(collection, (collection->val.l.count - 1)));
    }
  } else {
    size_t origsize = strlen(collection->val.strval);
    collection->val.strval[origsize - 1] = '\0';
    char *resized = realloc(collection->val.strval, origsize);
    if (resized == NULL) {
      perror("Could not resize char array");
      lvalue_del(v);
      lenvironment_del(e);
      exit(1);
    }
    collection->val.strval = resized;
  }

  lvalue_del(v);
  return collection;
}

/* * control flow builtins  * */

struct lvalue *builtin_exit(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "exit", 1);
  LARG_TYPE(v, "exit", 0, LVAL_INT);

  int exit_code = (int)(v->val.l.cells[0]->val.intval);
  lvalue_del(v);
  exit(exit_code);

  return lvalue_sexpr();
}

struct lvalue *builtin_if(struct lenvironment *e, struct lvalue *v) {
  LNUM_ARGS(v, "if", 3);
  LARG_TYPE(v, "if", 0, LVAL_BOOL);
  LARG_TYPE(v, "if", 1, LVAL_QEXPR);
  LARG_TYPE(v, "if", 2, LVAL_QEXPR);

  struct lvalue *res = NULL;
  struct lvalue *cond = v->val.l.cells[0];

  if (cond->val.intval) {
    res = lvalue_pop(v, 1);
    res->type = LVAL_SEXPR;
    res = lvalue_eval(e, res);
  } else {
    res = lvalue_pop(v, 2);
    res->type = LVAL_SEXPR;
    res = lvalue_eval(e, res);
  }

  lvalue_del(v);
  return res;
}

/* * reflection builtins * */

struct lvalue *builtin_type(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "type", 1);

  char *t = ltype_name(v->val.l.cells[0]->type);

  lvalue_del(v);
  return lvalue_str(t);
}

/* * function builtins * */

struct lvalue *builtin_lambda(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "\\", 2);
  LARG_TYPE(v, "\\", 0, LVAL_QEXPR);
  LARG_TYPE(v, "\\", 1, LVAL_QEXPR);

  struct lvalue *formals = v->val.l.cells[0];
  struct lvalue *body = v->val.l.cells[1];

  for (size_t i = 0; i < formals->val.l.count; ++i) {
    LASSERT(v, formals->val.l.cells[i]->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", i + 1,
            ltype_name(LVAL_SYM), ltype_name(formals->val.l.cells[i]->type));
  }

  formals = lvalue_pop(v, 0);
  body = lvalue_pop(v, 0);
  lvalue_del(v);

  return lvalue_lambda(formals, body, lambda_env_prealloc);
}

/**
 *  Function declaration builtin
 */
struct lvalue *builtin_fn(struct lenvironment *e, struct lvalue *v) {
  LNUM_ARGS(v, "fn", 3);
  LARG_TYPE(v, "fn", 0, LVAL_QEXPR); // function name
  LARG_TYPE(v, "fn", 1, LVAL_QEXPR); // parameter list
  LARG_TYPE(v, "fn", 2, LVAL_QEXPR); // body

  struct lvalue *name = LGETCELL(v, 0);
  struct lvalue *formals = LGETCELL(v, 1);
  struct lvalue *body = LGETCELL(v, 2);

  /* checking function name */
  LASSERT(v, name->val.l.count == 1,
          "Function was parsed a empty function name. A function must be named", 0);
  LASSERT(v, LGETCELL(name, 0)->type == LVAL_SYM,
          "Expected function name to be of type '%s'; got type '%s'.",
          ltype_name(LVAL_SYM), ltype_name(LGETCELL(name, 0)->type));

  /* checking formals parameters */
  for (size_t i = 0; i < formals->val.l.count; ++i) {
    LASSERT(v, LGETCELL(formals, i)->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", i + 1,
            ltype_name(LVAL_SYM), ltype_name(LGETCELL(formals, i)->type));
  }

  /* params are ok poping them off the value */
  name = lvalue_pop(v, 0);
  formals = lvalue_pop(v, 0);
  body = lvalue_pop(v, 0);

  struct lvalue *fn = lvalue_lambda(formals, body, fun_env_prealloc);

  lenvironment_put(e, LGETCELL(name, 0), fn);
  lvalue_del(fn);
  lvalue_del(name);
  lvalue_del(v);

  return lvalue_sexpr();
}

/* * value definition builtins * */

struct lvalue *builtin_var(struct lenvironment *, struct lvalue *, char *);

struct lvalue *builtin_def(struct lenvironment *e, struct lvalue *v) {
  return builtin_var(e, v, "def");
}

struct lvalue *builtin_put(struct lenvironment *e, struct lvalue *v) {
  return builtin_var(e, v, "=");
}

struct lvalue *builtin_var(struct lenvironment *e, struct lvalue *v, char *sym) {
  LARG_TYPE(v, sym, 0, LVAL_QEXPR);

  struct lvalue *names = LGETCELL(v, 0);

  for (size_t i = 0; i < names->val.l.count; ++i) {
    LASSERT(v, LGETCELL(names, i)->type == LVAL_SYM,
            "Function '%s' cannot assign value(s) to name(s). Name %lu is of type '%s'; "
            "expected type '%s'.",
            sym, i, ltype_name(LGETCELL(names, i)->type), ltype_name(LVAL_SYM));
  }

  LASSERT(v, names->val.l.count == v->val.l.count - 1,
          "Function '%s' cannot assign value(s) to name(s). Number of name(s) and "
          "value(s) does not match. Saw %lu name(s) expected %lu value(s).",
          sym, names->val.l.count, v->val.l.count - 1);

  for (size_t i = 0; i < names->val.l.count; ++i) {
    if (strcmp(sym, "def") == 0) {
      lenvironment_def(e, LGETCELL(names, i), LGETCELL(v, i + 1));
    } else if (strcmp(sym, "=") == 0) {
      lenvironment_put(e, LGETCELL(names, i), LGETCELL(v, i + 1));
    }
  }

  lvalue_del(v);
  return lvalue_sexpr();
}

/* * comparison builtins * */

struct lvalue *builtin_ord(struct lenvironment *e, struct lvalue *v, char *sym);

struct lvalue *builtin_lt(struct lenvironment *e, struct lvalue *v) {
  return builtin_ord(e, v, "<");
}

struct lvalue *builtin_gt(struct lenvironment *e, struct lvalue *v) {
  return builtin_ord(e, v, ">");
}

struct lvalue *builtin_le(struct lenvironment *e, struct lvalue *v) {
  return builtin_ord(e, v, "<=");
}

struct lvalue *builtin_ge(struct lenvironment *e, struct lvalue *v) {
  return builtin_ord(e, v, ">=");
}

struct lvalue *builtin_ord(struct lenvironment *e, struct lvalue *v, char *sym) {
  UNUSED(e);
  LNUM_ARGS(v, sym, 2);

  struct lvalue *lhs = v->val.l.cells[0];
  struct lvalue *rhs = v->val.l.cells[1];

  LASSERT(v, LIS_NUM(lhs->type),
          "Wrong type of argument parsed to '%s'. Expected '%s' or '%s' got '%s'.", sym,
          ltype_name(LVAL_INT), ltype_name(LVAL_FLOAT), ltype_name(lhs->type));
  LASSERT(v, LIS_NUM(rhs->type),
          "Wrong type of argument parsed to '%s'. Expected '%s' or '%s' got '%s'.", sym,
          ltype_name(LVAL_INT), ltype_name(LVAL_FLOAT), ltype_name(rhs->type));
  LASSERT(v, lhs->type == rhs->type,
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

  lvalue_del(v);
  return lvalue_bool(res);
}

struct lvalue *builtin_cmp(struct lenvironment *e, struct lvalue *v, char *sym) {
  UNUSED(e);
  LNUM_ARGS(v, sym, 2);

  struct lvalue *lhs = v->val.l.cells[0];
  struct lvalue *rhs = v->val.l.cells[1];

  long long res = 0;
  if (strcmp(sym, "==") == 0) {
    res = lvalue_eq(lhs, rhs);
  } else if (strcmp(sym, "!=") == 0) {
    res = !lvalue_eq(lhs, rhs);
  }

  lvalue_del(v);
  return lvalue_bool(res);
}

struct lvalue *builtin_eq(struct lenvironment *e, struct lvalue *v) {
  return builtin_cmp(e, v, "==");
}

struct lvalue *builtin_ne(struct lenvironment *e, struct lvalue *v) {
  return builtin_cmp(e, v, "!=");
}

/* logical builtins */

struct lvalue *builtin_and(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "&&", 2);
  LARG_TYPE(v, "&&", 0, LVAL_BOOL);
  LARG_TYPE(v, "&&", 1, LVAL_BOOL);

  long long res = (v->val.l.cells[0]->val.intval && v->val.l.cells[1]->val.intval);

  lvalue_del(v);
  return lvalue_bool(res);
}

struct lvalue *builtin_or(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "||", 2);
  LARG_TYPE(v, "||", 0, LVAL_BOOL);
  LARG_TYPE(v, "||", 1, LVAL_BOOL);

  long long res = (v->val.l.cells[0]->val.intval || v->val.l.cells[1]->val.intval);

  lvalue_del(v);
  return lvalue_bool(res);
}

struct lvalue *builtin_not(struct lenvironment *e, struct lvalue *v) {
  UNUSED(e);
  LNUM_ARGS(v, "!", 1);
  LARG_TYPE(v, "!", 0, LVAL_BOOL);

  long long res = !v->val.l.cells[0]->val.intval;

  lvalue_del(v);
  return lvalue_bool(res);
}

/* source importation builtins */

struct lvalue *builtin_load(struct lenvironment *e, struct lvalue *v) {
  LNUM_ARGS(v, "load", 1);
  LARG_TYPE(v, "load", 0, LVAL_STR);

  mpc_result_t r;
  if (mpc_parse_contents(LGETCELL(v, 0)->val.strval, elems.Lisper, &r)) {

    struct lvalue *expr = lvalue_read(r.output);
    mpc_ast_delete(r.output);

    while (expr->val.l.count) {
      struct lvalue *x = lvalue_eval(e, lvalue_pop(expr, 0));

      if (x->type == LVAL_ERR) {
        lvalue_println(x);
      }
      lvalue_del(x);
    }

    lvalue_del(expr);
    lvalue_del(v);

    return lvalue_sexpr();
  } else {
    /* parse error */
    char *err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);

    struct lvalue *err = lvalue_err("Could not load library %s", err_msg);
    free(err_msg);
    lvalue_del(v);

    return err;
  }
}

void register_builtins(struct lenvironment *e) {
  LENV_BUILTIN(list);
  LENV_BUILTIN(head);
  LENV_BUILTIN(tail);
  LENV_BUILTIN(eval);
  LENV_BUILTIN(join);
  LENV_BUILTIN(cons);
  LENV_BUILTIN(len);
  LENV_BUILTIN(init);
  LENV_BUILTIN(def);
  LENV_BUILTIN(exit);
  LENV_BUILTIN(max);
  LENV_BUILTIN(min);
  LENV_BUILTIN(fn);
  LENV_BUILTIN(if);
  LENV_BUILTIN(args);
  LENV_BUILTIN(type);
  LENV_BUILTIN(load);
  LENV_BUILTIN(error);
  LENV_BUILTIN(print);
  LENV_BUILTIN(read);
  LENV_BUILTIN(show);
  LENV_BUILTIN(open);
  LENV_BUILTIN(close);
  LENV_BUILTIN(flush);
  LENV_BUILTIN(putstr);
  LENV_BUILTIN(rewind);
  LENV_BUILTIN(getstr);

  LENV_SYMBUILTIN("+", add);
  LENV_SYMBUILTIN("-", sub);
  LENV_SYMBUILTIN("*", mul);
  LENV_SYMBUILTIN("/", div);
  LENV_SYMBUILTIN("%", mod);
  LENV_SYMBUILTIN("\\", lambda);
  LENV_SYMBUILTIN("=", put);

  LENV_SYMBUILTIN("==", eq);
  LENV_SYMBUILTIN("!=", ne);

  LENV_SYMBUILTIN("||", or);
  LENV_SYMBUILTIN("&&", and);
  LENV_SYMBUILTIN("!", not );

  LENV_SYMBUILTIN(">", gt);
  LENV_SYMBUILTIN("<", lt);
  LENV_SYMBUILTIN(">=", ge);
  LENV_SYMBUILTIN("<=", le);
}
