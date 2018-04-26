#ifndef LISPER_LENV
#define LISPER_LENV

#include "lval.h"
#include <stdlib.h>

struct lenv_t {
    size_t count;
    char** syms;
    lval_t** vals;
};

lenv_t *lenv_new(void);
void lenv_destroy(lenv_t *);

#endif

