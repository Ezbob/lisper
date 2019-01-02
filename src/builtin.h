#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lval.h"
#include "lenv.h"

struct lval_t *builtin_load(struct lenv_t *, struct lval_t *);

void register_builtins(struct lenv_t *e);

#endif

