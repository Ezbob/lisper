#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lval.h"
#include "lenv.h"

lval_t *lval_eval(lenv_t *, lval_t *);

lval_t *builtin_add(lenv_t *, lval_t *);
lval_t *builtin_sub(lenv_t *, lval_t *);
lval_t *builtin_mul(lenv_t *, lval_t *);
lval_t *builtin_div(lenv_t *, lval_t *);
lval_t *builtin_fmod(lenv_t *, lval_t *);
lval_t *builtin_pow(lenv_t *, lval_t *);
lval_t *builtin_min(lenv_t *, lval_t *);
lval_t *builtin_max(lenv_t *, lval_t *);

lval_t *builtin_tail(lenv_t *, lval_t *);
lval_t *builtin_head(lenv_t *, lval_t *);
lval_t *builtin_list(lenv_t *, lval_t *);
lval_t *builtin_eval(lenv_t *, lval_t *);
lval_t *builtin_join(lenv_t *, lval_t *);
lval_t *builtin_cons(lenv_t *, lval_t *);
lval_t *builtin_len(lenv_t *, lval_t *);
lval_t *builtin_init(lenv_t *, lval_t *);
lval_t *builtin_def(lenv_t *, lval_t *);
lval_t *builtin_exit(lenv_t *, lval_t *);
lval_t *builtin_lambda(lenv_t *, lval_t *);
lval_t *builtin_put(lenv_t *, lval_t *);

#endif

