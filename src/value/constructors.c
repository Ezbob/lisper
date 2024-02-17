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


extern struct mempool *lvalue_mp;

struct lvalue *lvalue_int(long long num) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_INT;
  val->val.intval = num;
  return val;
}

struct lvalue *lvalue_float(double num) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_FLOAT;
  val->val.floatval = num;
  return val;
}

struct lvalue *lvalue_bool(long long num) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_BOOL;
  val->val.intval = num;
  return val;
}

struct lvalue *lvalue_err(char *fmt, ...) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_ERR;
  va_list va;
  va_start(va, fmt);

  val->val.strval = malloc(512 * sizeof(char));
  vsnprintf(val->val.strval, 511, fmt, va);
  val->val.strval = realloc(val->val.strval, strlen(val->val.strval) + 1);

  va_end(va);
  return val;
}

struct lvalue *lvalue_sym(char *sym) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_SYM;
  val->val.strval = malloc(strlen(sym) + 1);
  strcpy(val->val.strval, sym);
  return val;
}

struct lvalue *lvalue_sexpr(void) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_SEXPR;
  val->val.l.count = 0;
  val->val.l.cells = NULL;
  return val;
}

struct lvalue *lvalue_qexpr(void) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_QEXPR;
  val->val.l.count = 0;
  val->val.l.cells = NULL;
  return val;
}

struct lvalue *lvalue_str(char *s) {
  struct lvalue *v = mempool_take(lvalue_mp);
  v->type = LVAL_STR;
  v->val.strval = malloc(strlen(s) + 1);
  strcpy(v->val.strval, s);
  return v;
}

struct lvalue *lvalue_builtin(struct lvalue *(*f)(struct lenvironment *,
                                                  struct lvalue *)) {
  struct lvalue *val = mempool_take(lvalue_mp);
  val->type = LVAL_BUILTIN;
  val->val.builtin = f;
  return val;
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

struct lvalue *lvalue_lambda(struct lvalue *formals, struct lvalue *body, size_t envcap) {
  struct lvalue *nw = mempool_take(lvalue_mp);
  nw->type = LVAL_FUNCTION;
  nw->val.fun = lfunc_new(lenvironment_new(envcap), formals, body);
  return nw;
}

struct lvalue *lvalue_file(struct lvalue *path, struct lvalue *mode, void *fp) {
  struct lvalue *nw = mempool_take(lvalue_mp);
  nw->type = LVAL_FILE;
  nw->val.file = lfile_new(path, mode, fp);
  return nw;
}
