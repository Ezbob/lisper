#ifndef LISPER_LENV
#define LISPER_LENV

#include "lval.h"
#include <stdlib.h>

typedef struct lenv_entry_t {
    char *name;
    lval_t *envval;
    struct lenv_entry_t *next;
} lenv_entry_t;

struct lenv_t {
    struct lenv_t *parent;
    lenv_entry_t **entries;
    size_t capacity;
};

lenv_t *lenv_new(size_t cap);
void lenv_del(lenv_t *);
lenv_t *lenv_copy(lenv_t *);
lval_t *lenv_get(lenv_t *, lval_t *);
void lenv_def(lenv_t *, lval_t *, lval_t *);
void lenv_put(lenv_t *, lval_t *, lval_t *);
void lenv_add_builtin(lenv_t *, char *, lbuiltin);
void lenv_pretty_print(lenv_t *);

#endif

