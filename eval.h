#ifndef LISPER_EVAL
#define LISPER_EVAL

#include "mpc.h"

typedef enum {
    LVAL_NUM,
    LVAL_ERR
} lres_t;

typedef enum {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
} lerr_t;

typedef struct {
    lres_t type;
    union {
        double num;
        lerr_t err;
    } val;
} lval_t;

void lval_print(lval_t *);
void lval_println(lval_t *);
lval_t eval(mpc_ast_t *);

#endif

