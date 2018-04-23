#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lval.h"

#define LASSERT(args, cond, err) \
    if ( !(cond) ) { lval_destroy(args); return lval_err(err);  }

lval_t *builtin_tail(lval_t *);
lval_t *builtin_head(lval_t *);
lval_t *builtin_list(lval_t *);
lval_t *builtin_eval(lval_t *);
lval_t *builtin_join(lval_t *);
lval_t *builtin_op(lval_t *, char *);

lval_t *builtin(lval_t *, char *);
lval_t *lval_eval(lval_t *);

#endif

