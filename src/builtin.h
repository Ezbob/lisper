#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lval.h"
#include "lenv.h"

lval_t *builtin(lval_t *, char *);
lval_t *lval_eval(lval_t *);

#endif

