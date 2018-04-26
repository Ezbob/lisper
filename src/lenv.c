#include "lenv.h"
#include <stdlib.h>

lenv_t *lenv_new(void) {
    lenv_t *env = malloc(sizeof(lenv));
    e->count = 0;
    e->syms = NULL;
    e->vals = NULL;
    return e;
}

void lenv_destroy(lenv_t *env) {
    for (size_t i = 0; i < env->count; ++i) {
        free(env->syms[i]);
        lval_destroy(env->vals[i]);
    }

    free(e->syms);
    free(e->vals);
    free(e);
}

