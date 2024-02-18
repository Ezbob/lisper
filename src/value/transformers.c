
#include "transformers.h"
#include "constructors.h"
#include "environment.h"
#include "lfile.h"
#include "lfunction.h"
#include "lvalue.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mempool.h"
#include "interpreter.h"

struct lvalue *builtin_list(struct linterpreter *intp, struct lvalue *v);
struct lvalue *builtin_eval(struct linterpreter *intp, struct lvalue *v);


struct lvalue *lvalue_add(struct mempool *mp, struct lvalue *val, struct lvalue *other) {
  val->val.l.count++;
  struct lvalue **resized_cells =
      realloc(val->val.l.cells, val->val.l.count * sizeof(struct lvalue *));
  if (resized_cells == NULL) {
    lvalue_del(mp, val);
    lvalue_del(mp, other);
    perror("Could not resize lvalue cell buffer");
    exit(1);
  }
  resized_cells[val->val.l.count - 1] = other;
  val->val.l.cells = resized_cells;
  return val;
}

struct lvalue *lvalue_offer(struct mempool *mp, struct lvalue *val, struct lvalue *other) {
  val->val.l.count++;
  struct lvalue **resized =
      realloc(val->val.l.cells, val->val.l.count * sizeof(struct lvalue *));
  // resize the memory buffer to carry another cell

  if (resized == NULL) {
    perror("Fatal memory error when trying reallocating for offer");
    lvalue_del(mp, val);
    exit(1);
  }
  val->val.l.cells = resized;
  memmove(val->val.l.cells + 1, val->val.l.cells,
          (val->val.l.count - 1) * sizeof(struct lvalue *));
  // move memory at address val->val.l.cells (up to old count of cells) to addr
  // val->val.l.cells[1]

  val->val.l.cells[0] = other;
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
  while (y->val.l.count) {
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
  struct lvalue *x = v->val.l.cells[i];
  memmove(v->val.l.cells + i, v->val.l.cells + (i + 1),
          sizeof(struct lvalue *) * (v->val.l.count - i - 1));
  v->val.l.count--;

  struct lvalue **cs = realloc(v->val.l.cells, v->val.l.count * sizeof(struct lvalue *));
  if (!cs && v->val.l.count > 0) {
    perror("Could not shrink cell buffer");
    lvalue_del(mp, v);
    exit(1);
  }
  v->val.l.cells = cs;

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
    x->val.fun =
        lfunc_new(lenvironment_copy(mp, v->val.fun->env), lvalue_copy(mp, v->val.fun->formals),
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
    x->val.strval = malloc((strlen(v->val.strval) + 1) * sizeof(char));
    strcpy(x->val.strval, v->val.strval);
    break;
  case LVAL_INT:
  case LVAL_BOOL:
    x->val.intval = v->val.intval;
    break;
  case LVAL_SEXPR:
  case LVAL_QEXPR:
    x->val.l.count = v->val.l.count;
    x->val.l.cells = malloc(v->val.l.count * sizeof(struct lvalue *));
    for (size_t i = 0; i < x->val.l.count; ++i) {
      x->val.l.cells[i] = lvalue_copy(mp, v->val.l.cells[i]);
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
    if (x->val.l.count != y->val.l.count) {
      return 0;
    }
    for (size_t i = 0; i < x->val.l.count; ++i) {
      if (!lvalue_eq(x->val.l.cells[i], y->val.l.cells[i])) {
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
struct lvalue *lvalue_call(struct linterpreter *intp, struct lvalue *f, struct lvalue *v) {
  struct lfunction *func = f->val.fun;
  struct lvalue **args = v->val.l.cells;
  struct lvalue *formals = func->formals;

  size_t given = v->val.l.count;
  size_t total = formals->val.l.count;

  while (v->val.l.count > 0) {

    if (formals->val.l.count == 0 && args[0]->type != LVAL_SEXPR) {
      /* Error case: non-symbolic parameter parsed */
      lvalue_del(&intp->lvalue_mp, v);
      return lvalue_err(&intp->lvalue_mp, "Function parsed too many arguments; "
                        "got %lu expected %lu",
                        given, total);
    } else if (formals->val.l.count == 0 && args[0]->type == LVAL_SEXPR &&
               args[0]->val.l.count == 0) {
      /* Function with no parameters case: can be called with a empty sexpr */
      break;
    }

    struct lvalue *sym = lvalue_pop(&intp->lvalue_mp, formals, 0); /* unbound name */

    if (strcmp(sym->val.strval, "&") == 0) {
      /* Variable argument case with '&' */
      if (formals->val.l.count != 1) {
        lvalue_del(&intp->lvalue_mp, v);
        return lvalue_err(&intp->lvalue_mp, "Function format invalid. "
                          "Symbol '&' not followed by a single symbol.");
      }

      /* Binding rest of the arguments to nsym */
      struct lvalue *nsym = lvalue_pop(&intp->lvalue_mp, formals, 0);
      lenvironment_put(&intp->lvalue_mp, func->env, nsym, builtin_list(intp, v));
      lvalue_del(&intp->lvalue_mp, sym);
      lvalue_del(&intp->lvalue_mp, nsym);
      break;
    }

    struct lvalue *val = lvalue_pop(&intp->lvalue_mp, v, 0); /* value to apply to unbound name */

    lenvironment_put(&intp->lvalue_mp, func->env, sym, val);
    lvalue_del(&intp->lvalue_mp, sym);
    lvalue_del(&intp->lvalue_mp, val);
  }

  lvalue_del(&intp->lvalue_mp, v);

  if (formals->val.l.count > 0 && strcmp(formals->val.l.cells[0]->val.strval, "&") == 0) {
    /* only first non-variable arguments was applied; create a empty qexpr  */

    if (formals->val.l.count != 2) {
      return lvalue_err(&intp->lvalue_mp, "Function format invalid. "
                        "Symbol '&' not followed by single symbol.");
    }

    /* Remove '&' */
    lvalue_del(&intp->lvalue_mp, lvalue_pop(&intp->lvalue_mp, formals, 0));

    /* Create a empty list for the next symbol */
    struct lvalue *sym = lvalue_pop(&intp->lvalue_mp, formals, 0);
    struct lvalue *val = lvalue_qexpr(&intp->lvalue_mp);

    lenvironment_put(&intp->lvalue_mp, func->env, sym, val);
    lvalue_del(&intp->lvalue_mp, sym);
    lvalue_del(&intp->lvalue_mp, val);
  }

  if (formals->val.l.count == 0) {
    func->env->parent = &intp->env;
    return builtin_eval(intp, lvalue_add(&intp->lvalue_mp, lvalue_sexpr(&intp->lvalue_mp), lvalue_copy(&intp->lvalue_mp, func->body)));
  }
  return lvalue_copy(&intp->lvalue_mp, f);
}

struct lvalue *lvalue_eval_sexpr(struct linterpreter *intp, struct lvalue *v);

struct lvalue *lvalue_eval(struct linterpreter *intp, struct lvalue *v) {

  struct lvalue *x;
  switch (v->type) {
  case LVAL_SYM:
    /* Eval'ing symbols just looks up the symbol in the symbol table
        Discards the wrapping.
      */
    x = lenvironment_get(&intp->lvalue_mp, &intp->env, v);
    lvalue_del(&intp->lvalue_mp, v);
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
  if (v->val.l.count == 0) {
    return v;
  }

  /*
   * Implicit quoting
   * Intercept special sexpr to make symbols quoted
   */
  struct lvalue *first = v->val.l.cells[0];
  if (first->type == LVAL_SYM && v->val.l.count > 1) {
    struct lvalue *sec = v->val.l.cells[1];
    if ((strcmp(first->val.strval, "=") == 0 || strcmp(first->val.strval, "def") == 0 ||
         strcmp(first->val.strval, "fn") == 0) &&
        sec->type == LVAL_SYM) {
      v->val.l.cells[1] = lvalue_add(&intp->lvalue_mp, lvalue_qexpr(&intp->lvalue_mp), sec);
    }
  }

  /* Depth-first evaluation of sexpr arguments.
      This resolves the actual meaning of the sexpr, such that
      what operator to apply to this sexpr is known, and if
      there was any error executing nested sexpr etc...
    */
  for (int i = 0; i < v->val.l.count; i++) {
    v->val.l.cells[i] = lvalue_eval(intp, v->val.l.cells[i]);
  }

  /* Hoist first lvalue if only one is available.
      This helps to sub results of sexprs, but
      the sub lval.has to be evaluated first.
    */
  if (v->val.l.count == 1) {
    return lvalue_take(&intp->lvalue_mp, v, 0);
  }

  /* Return first error (if any) */
  for (int i = 0; i < v->val.l.count; i++) {
    if (v->val.l.cells[i]->type == LVAL_ERR) {
      return lvalue_take(&intp->lvalue_mp, v, i);
    }
  }

  /* Function evaluation.
      Take the first lvalue in the sexpression and apply it to the
      following lvalue sequence
  */
  struct lvalue *operator= lvalue_pop(&intp->lvalue_mp, v, 0);
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
    lvalue_del(&intp->lvalue_mp, operator);
    lvalue_del(&intp->lvalue_mp, v);
    return lvalue_err(&intp->lvalue_mp, "Expected first argument of %s to be of type '%s'; got '%s'.",
                      ltype_name(LVAL_SEXPR), ltype_name(LVAL_BUILTIN), type_name);
  }

  lvalue_del(&intp->lvalue_mp, operator);
  return res;
}