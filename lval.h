#ifndef LISPER_EVAL
#define LISPER_EVAL

#include "mpc.h"
#include <stdlib.h>

typedef enum lres_t {
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR
} lres_t;

typedef enum lerr_t {
    LERR_DIV_ZERO,
    LERR_BAD_OP,
    LERR_BAD_NUM
} lerr_t;

struct lval_t; 

typedef struct lcell_list_t {
    size_t count;
    struct lval_t **cells;
} lcell_list_t;

typedef struct lval_t {
    lres_t type;
    union {
        double num;
        char *err;
        char *sym;
        lcell_list_t *l;
    } val;
} lval_t;

void lval_print(lval_t *);
void lval_println(lval_t *);
void lval_destroy(lval_t *);
lval_t *lval_read(mpc_ast_t *);

lval_t *lval_err(char *);
lval_t *lval_add(lval_t *, lval_t *);
lval_t *lval_pop(lval_t *, int);
lval_t *lval_take(lval_t *, int);

#endif

