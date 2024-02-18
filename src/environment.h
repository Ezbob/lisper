#ifndef LISPER_LENV
#define LISPER_LENV

#include "value.h"
#include <stdlib.h>
#include "mempool.h"

struct linterpreter;

typedef struct lvalue *(*builtin_func_ptr)(struct linterpreter *, struct lvalue *);

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

struct lenvironment *lenvironment_new(struct mempool *mp, size_t cap);
int lenvironment_init(struct mempool *mp, struct lenvironment *env, size_t capacity);
void lenvironment_deinit(struct mempool *mp, struct lenvironment *env);
void lenvironment_del(struct mempool *mp, struct lenvironment *);
struct lenvironment *lenvironment_copy(struct mempool *mp, struct lenvironment *);
struct lvalue *lenvironment_get(struct mempool *mp, struct lenvironment *, struct lvalue *);
void lenvironment_def(struct mempool *, struct lenvironment *, struct lvalue *, struct lvalue *);
void lenvironment_put(struct mempool *, struct lenvironment *, struct lvalue *, struct lvalue *);
void lenvironment_add_builtin(struct linterpreter *intp, char *, builtin_func_ptr);
void lenvironment_pretty_print(struct lenvironment *);

#endif
