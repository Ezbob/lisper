#include "constructors.h"
#include "environment.h"
#include "lfile.h"
#include "lfunction.h"
#include "lvalue.h"
#include "mempool.h"
#include "mpc.h"
#include "transformers.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct lvalue *lvalue_exit(struct mempool *mp, int rc) {
  struct lvalue *v = mempool_take(mp);
  v->type = LVAL_USER_EXIT;
  v->val.small_intval = rc;
  return v;
}

struct lvalue *lvalue_int(struct mempool *mp, long long num) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_INT;
  val->val.intval = num;
  return val;
}

struct lvalue *lvalue_float(struct mempool *mp, double num) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_FLOAT;
  val->val.floatval = num;
  return val;
}

struct lvalue *lvalue_bool(struct mempool *mp, long long num) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_BOOL;
  val->val.intval = num;
  return val;
}

struct lvalue *lvalue_err(struct mempool *mp, char *fmt, ...) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_ERR;
  va_list va;
  va_start(va, fmt);

  val->val.strval = malloc(512 * sizeof(char));
  vsnprintf(val->val.strval, 511, fmt, va);
  val->val.strval = realloc(val->val.strval, strlen(val->val.strval) + 1);

  va_end(va);
  return val;
}

struct lvalue *lvalue_sym(struct mempool *mp, char *sym) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_SYM;
  val->val.strval = malloc(strlen(sym) + 1);
  strcpy(val->val.strval, sym);
  return val;
}

struct lvalue *lvalue_sexpr(struct mempool *mp) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_SEXPR;
  val->val.list.count = 0;
  val->val.list.cells = NULL;
  return val;
}

struct lvalue *lvalue_qexpr(struct mempool *mp) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_QEXPR;
  val->val.list.count = 0;
  val->val.list.cells = NULL;
  return val;
}

struct lvalue *lvalue_str(struct mempool *mp, char *s) {
  struct lvalue *v = mempool_take(mp);
  v->type = LVAL_STR;
  v->val.strval = malloc(strlen(s) + 1);
  strcpy(v->val.strval, s);
  return v;
}

struct lvalue *lvalue_builtin(struct mempool *mp,
                              struct lvalue *(*f)(struct linterpreter *,
                                                  struct lvalue *)) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_BUILTIN;
  val->val.builtin = f;
  return val;
}

struct lvalue *lvalue_lambda(struct mempool *mp, struct lvalue *formals,
                             struct lvalue *body, size_t envcap) {
  struct lvalue *nw = mempool_take(mp);
  nw->type = LVAL_FUNCTION;
  nw->val.fun = lfunc_new(lenvironment_new(mp, envcap), formals, body);
  return nw;
}

struct lvalue *lvalue_file(struct mempool *mp, struct lvalue *path, struct lvalue *mode,
                           void *fp) {
  struct lvalue *nw = mempool_take(mp);
  nw->type = LVAL_FILE;
  nw->val.file = lfile_new(path, mode, fp);
  return nw;
}

struct lfunction *lfunc_new(struct lenvironment *env, struct lvalue *formals,
                            struct lvalue *body) {
  struct lfunction *new = malloc(sizeof(struct lfunction));
  new->name = NULL;
  new->env = env;
  new->formals = formals;
  new->body = body;
  return new;
}

struct lfile *lfile_new(struct lvalue *path, struct lvalue *mode, void *fp) {
  struct lfile *new = malloc(sizeof(struct lfile));
  new->path = path;
  new->mode = mode;
  new->fp = fp;
  return new;
}

struct lvalue *lvalue_read_int(struct mempool *mp, mpc_ast_t *t) {
  long long int int_read = 0;

  int code = sscanf(t->contents, "%lli", &int_read);
  if (code != 1) {
    return lvalue_err(mp, "Cloud not parse '%s' as a number.", t->contents);
  }

  return lvalue_int(mp, int_read);
}

struct lvalue *lvalue_read_float(struct mempool *mp, mpc_ast_t *t) {
  double float_read = 0.0;
  int code = sscanf(t->contents, "%lf", &float_read);
  if (code != 1) {
    return lvalue_err(mp, "Cloud not parse '%s' as a number.", t->contents);
  }
  return lvalue_float(mp, float_read);
}

struct lvalue *lvalue_read_str(struct mempool *mp, mpc_ast_t *t) {
  t->contents[strlen(t->contents) - 1] = '\0';
  /* clip off the newline */

  char *unescaped = malloc(strlen(t->contents + 1) + 1);
  strcpy(unescaped, t->contents + 1);
  /*  but make room for the newline in the unescaped output */

  unescaped = mpcf_unescape(unescaped);
  /* unescape probably inserts a newline into the string */
  struct lvalue *str = lvalue_str(mp, unescaped);

  free(unescaped);
  return str;
}

struct lvalue *lvalue_read(struct mempool *mp, mpc_ast_t *t) {
  struct lvalue *val = NULL;

  if (strstr(t->tag, "boolean")) {
    long long res = (strcmp(t->contents, "true") == 0) ? 1 : 0;
    return lvalue_bool(mp, res);
  }

  if (strstr(t->tag, "string")) {
    return lvalue_read_str(mp, t);
  }

  if (strstr(t->tag, "float")) {
    return lvalue_read_float(mp, t);
  }

  if (strstr(t->tag, "integer")) {
    return lvalue_read_int(mp, t);
  }

  if (strstr(t->tag, "symbol")) {
    return lvalue_sym(mp, t->contents);
  }

  if (strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr")) {
    /* ">" is the root of the AST */
    val = lvalue_sexpr(mp);
  }

  if (strstr(t->tag, "qexpr")) {
    val = lvalue_qexpr(mp);
  }

  for (int i = 0; i < t->children_num; i++) {
    struct mpc_ast_t *child = t->children[i];
    if (strcmp(child->contents, "(") == 0 || strcmp(child->contents, ")") == 0 ||
        strcmp(child->contents, "{") == 0 || strcmp(child->contents, "}") == 0 ||
        strcmp(child->tag, "regex") == 0 || strstr(child->tag, "comment")) {
      continue;
    }
    val = lvalue_add(mp, val, lvalue_read(mp, child));
  }

  return val;
}

void lvalue_del(struct mempool *mp, struct lvalue *val) {
  struct lfile *file;
  struct lfunction *func;
  switch (val->type) {
  case LVAL_FLOAT:
  case LVAL_INT:
  case LVAL_BUILTIN:
  case LVAL_BOOL:
    break;
  case LVAL_FUNCTION:
    func = val->val.fun;
    lenvironment_del(mp, func->env);
    lvalue_del(mp, func->formals);
    lvalue_del(mp, func->body);
    free(func);
    break;
  case LVAL_FILE:
    file = val->val.file;
    lvalue_del(mp, file->path);
    lvalue_del(mp, file->mode);
    free(file);
    break;
  case LVAL_ERR:
  case LVAL_SYM:
  case LVAL_STR:
    free(val->val.strval);
    break;
  case LVAL_QEXPR:
  case LVAL_SEXPR:
    for (size_t i = 0; i < val->val.list.count; ++i) {
      lvalue_del(mp, val->val.list.cells[i]);
    }
    free(val->val.list.cells);
    break;
  }
  mempool_recycle(mp, val);
}
