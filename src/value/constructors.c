#include "constructors.h"
#include "environment.h"
#include "lfile.h"
#include "lfunction.h"
#include "lvalue.h"
#include "mempool.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
  val->val.l.count = 0;
  val->val.l.cells = NULL;
  return val;
}

struct lvalue *lvalue_qexpr(struct mempool *mp) {
  struct lvalue *val = mempool_take(mp);
  val->type = LVAL_QEXPR;
  val->val.l.count = 0;
  val->val.l.cells = NULL;
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
