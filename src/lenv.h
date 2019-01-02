#ifndef LISPER_LENV
#define LISPER_LENV

#include "lval.h"
#include <stdlib.h>

struct lenv_entry_t {
    char *name;
    struct lval_t *envval;
    struct lenv_entry_t *next;
};

struct lenv_t {
    struct lenv_t *parent;
    struct lenv_entry_t **entries;
    size_t capacity;
};

struct lenv_t *lenv_new(size_t cap);
void lenv_del(struct lenv_t *);
struct lenv_t *lenv_copy(struct lenv_t *);
struct lval_t *lenv_get(struct lenv_t *, struct lval_t *);
void lenv_def(struct lenv_t *, struct lval_t *, struct lval_t *);
void lenv_put(struct lenv_t *, struct lval_t *, struct lval_t *);
void lenv_add_builtin(struct lenv_t *, char *, lbuiltin);
void lenv_pretty_print(struct lenv_t *);

#endif

