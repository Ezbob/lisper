
#include "transformers.h"
#include "constructors.h"
#include "environment.h"
#include "lfile.h"
#include "lfunction.h"
#include "lvalue.h"
#include <stdio.h>
#include <stdlib.h>


extern struct mempool *lvalue_mp;

struct lvalue *lvalue_add(struct lvalue *val, struct lvalue *other) {
  val->val.l.count++;
  struct lvalue **resized_cells =
      realloc(val->val.l.cells, val->val.l.count * sizeof(struct lvalue *));
  if (resized_cells == NULL) {
    lvalue_del(val);
    lvalue_del(other);
    perror("Could not resize lvalue cell buffer");
    exit(1);
  }
  resized_cells[val->val.l.count - 1] = other;
  val->val.l.cells = resized_cells;
  return val;
}

struct lvalue *lvalue_offer(struct lvalue *val, struct lvalue *other) {
  val->val.l.count++;
  struct lvalue **resized =
      realloc(val->val.l.cells, val->val.l.count * sizeof(struct lvalue *));
  // resize the memory buffer to carry another cell

  if (resized == NULL) {
    perror("Fatal memory error when trying reallocating for offer");
    lvalue_del(val);
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

struct lvalue *lvalue_join_str(struct lvalue *x, struct lvalue *y) {
  size_t cpy_start = strlen(x->val.strval);
  size_t total_size = cpy_start + strlen(y->val.strval);

  char *resized = realloc(x->val.strval, total_size + 1);
  if (resized == NULL) {
    perror("Fatal memory error when trying reallocating for join_str");
    lvalue_del(x);
    lvalue_del(y);
    exit(1);
  }
  x->val.strval = resized;
  strcpy(x->val.strval + cpy_start, y->val.strval);

  lvalue_del(y);
  return x;
}

struct lvalue *lvalue_join(struct lvalue *x, struct lvalue *y) {
  while (y->val.l.count) {
    x = lvalue_add(x, lvalue_pop(y, 0));
  }

  lvalue_del(y);
  return x;
}

/**
 * Pops the value of the input lvalue at index i,
 * and resizes the memory buffer
 */
struct lvalue *lvalue_pop(struct lvalue *v, int i) {
  struct lvalue *x = v->val.l.cells[i];
  memmove(v->val.l.cells + i, v->val.l.cells + (i + 1),
          sizeof(struct lvalue *) * (v->val.l.count - i - 1));
  v->val.l.count--;

  struct lvalue **cs = realloc(v->val.l.cells, v->val.l.count * sizeof(struct lvalue *));
  if (!cs && v->val.l.count > 0) {
    perror("Could not shrink cell buffer");
    lvalue_del(v);
    exit(1);
  }
  v->val.l.cells = cs;

  return x;
}

/**
 * Pop the value off the input value at index i, and
 * delete the input value
 */
struct lvalue *lvalue_take(struct lvalue *v, int i) {
  struct lvalue *x = lvalue_pop(v, i);
  lvalue_del(v);
  return x;
}

/**
 * Create a copy of the input lvalue
 */
struct lvalue *lvalue_copy(struct lvalue *v) {
  
  struct lvalue *x = mempool_take(lvalue_mp);
  x->type = v->type;
  struct lvalue *p;
  FILE *fp;
  struct lvalue *m;

  switch (v->type) {
  case LVAL_FUNCTION:
    x->val.fun =
        lfunc_new(lenvironment_copy(v->val.fun->env), lvalue_copy(v->val.fun->formals),
                  lvalue_copy(v->val.fun->body));
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
      x->val.l.cells[i] = lvalue_copy(v->val.l.cells[i]);
    }
    break;
  case LVAL_FILE:
    p = lvalue_copy(v->val.file->path);
    m = lvalue_copy(v->val.file->mode);
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
struct lvalue *lvalue_call(struct lenvironment *e, struct lvalue *f, struct lvalue *v) {
  if (f->type == LVAL_BUILTIN) {
    return f->val.builtin(e, v);
  }

  struct lfunction *func = f->val.fun;
  struct lvalue **args = v->val.l.cells;
  struct lvalue *formals = func->formals;

  size_t given = v->val.l.count;
  size_t total = formals->val.l.count;

  while (v->val.l.count > 0) {

    if (formals->val.l.count == 0 && args[0]->type != LVAL_SEXPR) {
      /* Error case: non-symbolic parameter parsed */
      lvalue_del(v);
      return lvalue_err("Function parsed too many arguments; "
                        "got %lu expected %lu",
                        given, total);
    } else if (formals->val.l.count == 0 && args[0]->type == LVAL_SEXPR &&
               args[0]->val.l.count == 0) {
      /* Function with no parameters case: can be called with a empty sexpr */
      break;
    }

    struct lvalue *sym = lvalue_pop(formals, 0); /* unbound name */

    if (strcmp(sym->val.strval, "&") == 0) {
      /* Variable argument case with '&' */
      if (formals->val.l.count != 1) {
        lvalue_del(v);
        return lvalue_err("Function format invalid. "
                          "Symbol '&' not followed by a single symbol.");
      }

      /* Binding rest of the arguments to nsym */
      struct lvalue *nsym = lvalue_pop(formals, 0);
      lenvironment_put(func->env, nsym, builtin_list(e, v));
      lvalue_del(sym);
      lvalue_del(nsym);
      break;
    }

    struct lvalue *val = lvalue_pop(v, 0); /* value to apply to unbound name */

    lenvironment_put(func->env, sym, val);
    lvalue_del(sym);
    lvalue_del(val);
  }

  lvalue_del(v);

  if (formals->val.l.count > 0 && strcmp(formals->val.l.cells[0]->val.strval, "&") == 0) {
    /* only first non-variable arguments was applied; create a empty qexpr  */

    if (formals->val.l.count != 2) {
      return lvalue_err("Function format invalid. "
                        "Symbol '&' not followed by single symbol.");
    }

    /* Remove '&' */
    lvalue_del(lvalue_pop(formals, 0));

    /* Create a empty list for the next symbol */
    struct lvalue *sym = lvalue_pop(formals, 0);
    struct lvalue *val = lvalue_qexpr();

    lenvironment_put(func->env, sym, val);
    lvalue_del(sym);
    lvalue_del(val);
  }

  if (formals->val.l.count == 0) {
    func->env->parent = e;
    return builtin_eval(func->env, lvalue_add(lvalue_sexpr(), lvalue_copy(func->body)));
  }
  return lvalue_copy(f);
}

struct lvalue *lvalue_eval(struct lenvironment *e, struct lvalue *v) {

  struct lvalue *x;
  switch (v->type) {
  case LVAL_SYM:
    /* Eval'ing symbols just looks up the symbol in the symbol table
        Discards the wrapping.
      */
    x = lenvironment_get(e, v);
    lvalue_del(v);
    return x;

  case LVAL_SEXPR:
    /* this might by a nested sexpr or toplevel */
    return lvalue_eval_sexpr(e, v);
  default:
    break;
  }

  return v;
}
