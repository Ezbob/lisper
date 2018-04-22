#ifndef LISPER_EVAL
#define LISPER_EVAL

#include "mpc.h"
#include <stdlib.h>

typedef enum lres_t {
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR
} lres_t;

typedef enum lerr_t {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
} lerr_t;

struct lval_t; 

typedef struct lscell_t {
    size_t count;
    struct lval_t **cells;
} lscell_t;

typedef struct lval_t {
    lres_t type;
    union {
        double num;
        char *err;
        char *sym;
        lscell_t *symcells;
    } val;
} lval_t;

void lval_print(lval_t *);
void lval_println(lval_t *);
lval_t *lval_read(mpc_ast_t *);
void lval_destroy(lval_t *);

#endif

