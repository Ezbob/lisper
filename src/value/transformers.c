
#include "transformers.h"
#include "constructors.h"
#include "environment.h"
#include "lisper.h"
#include "lfile.h"
#include "lfunction.h"
#include "interpreter.h"
#include "lvalue.h"
#include "mempool.h"
#include "print.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compat/string.h"

struct lvalue *builtin_list(struct linterpreter *intp, struct lvalue *v);
struct lvalue *builtin_eval(struct linterpreter *intp, struct lvalue *v);

struct lvalue *lvalue_add(struct mempool *mp, struct lvalue *val, struct lvalue *other) {
  val->val.list.count++;
  struct lvalue **resized_cells =
      realloc(val->val.list.cells, val->val.list.count * sizeof(struct lvalue *));
  if (resized_cells == NULL) {
    lvalue_del(mp, val);
    lvalue_del(mp, other);
    perror("Could not resize lvalue cell buffer");
    exit(1);
  }
  resized_cells[val->val.list.count - 1] = other;
  val->val.list.cells = resized_cells;
  return val;
}

struct lvalue *lvalue_offer(struct mempool *mp, struct lvalue *val,
                            struct lvalue *other) {
  val->val.list.count++;
  struct lvalue **resized =
      realloc(val->val.list.cells, val->val.list.count * sizeof(struct lvalue *));
  // resize the memory buffer to carry another cell

  if (resized == NULL) {
    perror("Fatal memory error when trying reallocating for offer");
    lvalue_del(mp, val);
    exit(1);
  }
  val->val.list.cells = resized;
  memmove(val->val.list.cells + 1, val->val.list.cells,
          (val->val.list.count - 1) * sizeof(struct lvalue *));
  // move memory at address val->val.list.cells (up to old count of cells) to addr
  // val->val.list.cells[1]

  val->val.list.cells[0] = other;
  // insert into the front of the array

  return val;
}

struct lvalue *lvalue_join_str(struct mempool *mp, struct lvalue *x, struct lvalue *y) {
  size_t cpy_start = strlen(x->val.strval);
  size_t total_size = cpy_start + strlen(y->val.strval);

  char *resized = realloc(x->val.strval, total_size + 1);
  if (resized == NULL) {
    perror("Fatal memory error when trying reallocating for join_str");
    lvalue_del(mp, x);
    lvalue_del(mp, y);
    exit(1);
  }
  x->val.strval = resized;
  strcpy(x->val.strval + cpy_start, y->val.strval);

  lvalue_del(mp, y);
  return x;
}

struct lvalue *lvalue_join(struct mempool *mp, struct lvalue *x, struct lvalue *y) {
  while (y->val.list.count) {
    x = lvalue_add(mp, x, lvalue_pop(mp, y, 0));
  }

  lvalue_del(mp, y);
  return x;
}

/**
 * Pops the value of the input lvalue at index i,
 * and resizes the memory buffer
 */
struct lvalue *lvalue_pop(struct mempool *mp, struct lvalue *v, int i) {
  struct lvalue *x = v->val.list.cells[i];
  memmove(v->val.list.cells + i, v->val.list.cells + (i + 1),
          sizeof(struct lvalue *) * (v->val.list.count - i - 1));
  v->val.list.count--;

  struct lvalue **cs =
      realloc(v->val.list.cells, v->val.list.count * sizeof(struct lvalue *));
  if (!cs && v->val.list.count > 0) {
    perror("Could not shrink cell buffer");
    lvalue_del(mp, v);
    exit(1);
  }
  v->val.list.cells = cs;

  return x;
}

/**
 * Pop the value off the input value at index i, and
 * delete the input value
 */
struct lvalue *lvalue_take(struct mempool *mp, struct lvalue *v, int i) {
  struct lvalue *x = lvalue_pop(mp, v, i);
  lvalue_del(mp, v);
  return x;
}

/**
 * Create a copy of the input lvalue
 */
struct lvalue *lvalue_copy(struct mempool *mp, struct lvalue *v) {

  struct lvalue *x = mempool_take(mp);
  x->type = v->type;
  struct lvalue *p;
  FILE *fp;
  struct lvalue *m;

  switch (v->type) {
  case LVAL_FUNCTION:
    x->val.fun = lfunc_new(lenvironment_copy(mp, v->val.fun->env),
                           lvalue_copy(mp, v->val.fun->formals),
                           lvalue_copy(mp, v->val.fun->body));
    break;
  case LVAL_BUILTIN:
    x->val.builtin = v->val.builtin;
    break;
  case LVAL_FLOAT:
    x->val.floatval = v->val.floatval;
    break;
  case LVAL_ERR:
  case LVAL_SYM:
  case LVAL_STR:
    x->val.strval = strdup(v->val.strval);
    break;
  case LVAL_USER_EXIT:
    x->val.small_intval = v->val.small_intval;
    break;
  case LVAL_INT:
  case LVAL_BOOL:
    x->val.intval = v->val.intval;
    break;
  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->val.list.count = v->val.list.count;
    x->val.list.cells = malloc(v->val.list.count * sizeof(struct lvalue *));
    for (int i = 0; i < x->val.list.count; ++i) {
      x->val.list.cells[i] = lvalue_copy(mp, v->val.list.cells[i]);
    }
    break;
  case LVAL_FILE:
    p = lvalue_copy(mp, v->val.file->path);
    m = lvalue_copy(mp, v->val.file->mode);
    fp = v->val.file->fp; /* copy share fp to limit fp use to the same file */
    x->val.file = lfile_new(p, m, fp);
    break;
  }

  return x;
}

/**
 * Compare two lvalues for equality.
 * Returns zero if input values x and y are not equal,
 * returns a non-zero otherwise
 */
int lvalue_eq(struct lvalue *x, struct lvalue *y) {
  if (x->type != y->type) {
    return 0;
  }

  switch (x->type) {
  case LVAL_FLOAT:
    return (x->val.floatval == y->val.floatval);
  case LVAL_BOOL:
  case LVAL_INT:
    return (x->val.intval == y->val.intval);
  case LVAL_USER_EXIT:
    return x->val.small_intval == y->val.small_intval;
  case LVAL_ERR:
  case LVAL_SYM:
  case LVAL_STR:
    return strcmp(x->val.strval, y->val.strval) == 0;
  case LVAL_BUILTIN:
    return (x->val.builtin == y->val.builtin);
  case LVAL_FUNCTION:
    return lvalue_eq(x->val.fun->formals, y->val.fun->formals) &&
           lvalue_eq(x->val.fun->body, y->val.fun->body);
  case LVAL_FILE:
    return lvalue_eq(x->val.file->path, y->val.file->path) &&
           lvalue_eq(x->val.file->mode, y->val.file->mode) &&
           x->val.file->fp == y->val.file->fp;
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    if (x->val.list.count != y->val.list.count) {
      return 0;
    }
    for (int i = 0; i < x->val.list.count; ++i) {
      if (!lvalue_eq(x->val.list.cells[i], y->val.list.cells[i])) {
        return 0;
      }
    }
    return 1;
  }
  return 0;
}

/**
 * evaluate a function
 */
struct lvalue *lvalue_call(struct linterpreter *intp, struct lvalue *f,
                           struct lvalue *v) {
  struct lfunction *func = f->val.fun;
  struct lvalue **args = v->val.list.cells;
  struct lvalue *formals = func->formals;

  size_t given = v->val.list.count;
  size_t total = formals->val.list.count;

  while (v->val.list.count > 0) {

    if (formals->val.list.count == 0 && args[0]->type != LVAL_SEXPR) {
      /* Error case: non-symbolic parameter parsed */
      lvalue_del(intp->lvalue_mp, v);
      return lvalue_err(intp->lvalue_mp,
                        "Function parsed too many arguments; "
                        "got %lu expected %lu",
                        given, total);
    } else if (formals->val.list.count == 0 && args[0]->type == LVAL_SEXPR &&
               args[0]->val.list.count == 0) {
      /* Function with no parameters case: can be called with a empty sexpr */
      break;
    }

    struct lvalue *sym = lvalue_pop(intp->lvalue_mp, formals, 0); /* unbound name */

    if (strcmp(sym->val.strval, "&") == 0) {
      /* Variable argument case with '&' */
      if (formals->val.list.count != 1) {
        lvalue_del(intp->lvalue_mp, v);
        return lvalue_err(intp->lvalue_mp,
                          "Function format invalid. "
                          "Symbol '&' not followed by a single symbol.");
      }

      /* Binding rest of the arguments to nsym */
      struct lvalue *nsym = lvalue_pop(intp->lvalue_mp, formals, 0);
      lenvironment_put(intp->lvalue_mp, func->env, nsym, builtin_list(intp, v));
      lvalue_del(intp->lvalue_mp, sym);
      lvalue_del(intp->lvalue_mp, nsym);
      break;
    }

    struct lvalue *val =
        lvalue_pop(intp->lvalue_mp, v, 0); /* value to apply to unbound name */

    lenvironment_put(intp->lvalue_mp, func->env, sym, val);
    lvalue_del(intp->lvalue_mp, sym);
    lvalue_del(intp->lvalue_mp, val);
  }

  lvalue_del(intp->lvalue_mp, v);

  if (formals->val.list.count > 0 &&
      strcmp(formals->val.list.cells[0]->val.strval, "&") == 0) {
    /* only first non-variable arguments was applied; create a empty qexpr  */

    if (formals->val.list.count != 2) {
      return lvalue_err(intp->lvalue_mp, "Function format invalid. "
                                          "Symbol '&' not followed by single symbol.");
    }

    /* Remove '&' */
    lvalue_del(intp->lvalue_mp, lvalue_pop(intp->lvalue_mp, formals, 0));

    /* Create a empty list for the next symbol */
    struct lvalue *sym = lvalue_pop(intp->lvalue_mp, formals, 0);
    struct lvalue *val = lvalue_qexpr(intp->lvalue_mp);

    lenvironment_put(intp->lvalue_mp, func->env, sym, val);
    lvalue_del(intp->lvalue_mp, sym);
    lvalue_del(intp->lvalue_mp, val);
  }

  if (formals->val.list.count == 0) {
    func->env->parent = intp->env;
    return builtin_eval(intp, lvalue_add(intp->lvalue_mp, lvalue_sexpr(intp->lvalue_mp),
                                         lvalue_copy(intp->lvalue_mp, func->body)));
  }
  return lvalue_copy(intp->lvalue_mp, f);
}

struct lvalue *lvalue_eval_sexpr(struct linterpreter *intp, struct lvalue *v);

struct lvalue *lvalue_eval(struct linterpreter *intp, struct lvalue *v) {

  struct lvalue *x;
  switch (v->type) {
  case LVAL_SYM:
    /* Eval'ing symbols just looks up the symbol in the symbol table
        Discards the wrapping.
      */
    x = lenvironment_get(intp->lvalue_mp, intp->env, v);
    lvalue_del(intp->lvalue_mp, v);
    return x;

  case LVAL_SEXPR:
    /* this might by a nested sexpr or toplevel */
    return lvalue_eval_sexpr(intp, v);
  default:
    break;
  }

  return v;
}

struct lvalue *lvalue_eval_sexpr(struct linterpreter *intp, struct lvalue *v) {
  /* empty sexpr */
  if (v->val.list.count == 0) {
    return v;
  }

  /*
   * Implicit quoting
   * Intercept special sexpr to make symbols quoted
   */
  struct lvalue *first = v->val.list.cells[0];
  if (first->type == LVAL_SYM && v->val.list.count > 1) {
    struct lvalue *sec = v->val.list.cells[1];
    if ((strcmp(first->val.strval, "=") == 0 || strcmp(first->val.strval, "def") == 0 ||
         strcmp(first->val.strval, "fn") == 0) &&
        sec->type == LVAL_SYM) {
      v->val.list.cells[1] =
          lvalue_add(intp->lvalue_mp, lvalue_qexpr(intp->lvalue_mp), sec);
    }
  }

  /* Depth-first evaluation of sexpr arguments.
      This resolves the actual meaning of the sexpr, such that
      what operator to apply to this sexpr is known, and if
      there was any error executing nested sexpr etc...
    */
  for (int i = 0; i < v->val.list.count; i++) {
    struct lvalue *res = lvalue_eval(intp, v->val.list.cells[i]);
    if (intp->halt_type == LINTERP_USER_EXIT && res->type == LVAL_USER_EXIT) {
      lvalue_del(intp->lvalue_mp, v);
      return res;
    }
    v->val.list.cells[i] = res;
  }

  /* Hoist first lvalue if only one is available.
      This helps to sub results of sexprs, but
      the sub lval.has to be evaluated first.
    */
  if (v->val.list.count == 1) {
    return lvalue_take(intp->lvalue_mp, v, 0);
  }

  /* Return first error (if any) */
  for (int i = 0; i < v->val.list.count; i++) {
    if (v->val.list.cells[i]->type == LVAL_ERR) {
      return lvalue_take(intp->lvalue_mp, v, i);
    }
  }

  /* Function evaluation.
      Take the first lvalue in the sexpression and apply it to the
      following lvalue sequence
  */
  struct lvalue *operator= lvalue_pop(intp->lvalue_mp, v, 0);
  struct lvalue *res = NULL;
  char *type_name = NULL;

  switch (operator->type) {
  case LVAL_BUILTIN:
    res = operator->val.builtin(intp, v);
    break;
  case LVAL_FUNCTION:
    res = lvalue_call(intp, operator, v);
    break;
  default:
    type_name = ltype_name(operator->type);
    lvalue_del(intp->lvalue_mp, operator);
    lvalue_del(intp->lvalue_mp, v);
    return lvalue_err(intp->lvalue_mp,
                      "Expected first argument of %s to be of type '%s'; got '%s'.",
                      ltype_name(LVAL_SEXPR), ltype_name(LVAL_BUILTIN), type_name);
  }

  lvalue_del(intp->lvalue_mp, operator);
  return res;
}
