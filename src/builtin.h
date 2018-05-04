#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lval.h"
#include "lenv.h"

lval_t *builtin_load(lenv_t *, lval_t *);

void register_builtins(lenv_t *e);

#endif

