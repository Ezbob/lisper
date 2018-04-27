#ifndef LISPER_LENV
#define LISPER_LENV

#include "lval.h"
#include <stdlib.h>

struct lenv_t {
    size_t count;
    char** syms;
    lval_t** vals;
};

lenv_t *lenv_new(void);
void lenv_del(lenv_t *);
lval_t *lenv_get(lenv_t *, lval_t *);
void lenv_put(lenv_t *, lval_t *, lval_t *);
void lenv_add_builtin(lenv_t *, char *, lbuiltin);
void lenv_add_builtins(lenv_t *);
void lenv_pretty_print(lenv_t *);

#endif

