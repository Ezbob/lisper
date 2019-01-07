#ifndef LISPER_LENV
#define LISPER_LENV

#include "lvalue.h"
#include <stdlib.h>

struct lenvironment_entry {
    char *name;
    struct lvalue *envval;
    struct lenvironment_entry *next;
};

struct lenvironment {
    struct lenvironment *parent;
    struct lenvironment_entry **entries;
    size_t capacity;
};

struct lenvironment *lenvironment_new(size_t cap);
void lenvironment_del(struct lenvironment *);
struct lenvironment *lenvironment_copy(struct lenvironment *);
struct lvalue *lenvironment_get(struct lenvironment *, struct lvalue *);
void lenvironment_def(struct lenvironment *, struct lvalue *, struct lvalue *);
void lenvironment_put(struct lenvironment *, struct lvalue *, struct lvalue *);
void lenvironment_add_builtin(struct lenvironment *, char *, struct lvalue *(*)(struct lenvironment *, struct lvalue *));
void lenvironment_pretty_print(struct lenvironment *);

#endif

