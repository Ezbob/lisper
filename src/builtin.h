#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lval.h"

#define LASSERT(args, cond, err) \
    if ( !(cond) ) { lval_destroy(args); return lval_err(err);  }

lval_t *builtin(lval_t *, char *);
lval_t *lval_eval(lval_t *);

#endif

