#ifndef LISPER_EVAL
#define LISPER_EVAL

#include "mpc.h"
#include <stdlib.h>

typedef enum ltype {
    LVAL_NUM,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_FUN
} ltype;

struct lval_t; 
struct lenv_t;

typedef struct lval_t lval_t;
typedef struct lenv_t lenv_t;

typedef lval_t *(*lbuiltin)(lenv_t *, lval_t *);

typedef struct lcell_list_t {
    size_t count;
    struct lval_t **cells;
} lcell_list_t;

struct lval_t {
    ltype type;
    union val {
        double num;
        char *err;
        char *sym;
        lcell_list_t *l;
        lbuiltin fun;
    } val;
};

lval_t *lval_read(mpc_ast_t *);

void lval_print(lval_t *);
void lval_println(lval_t *);
void lval_del(lval_t *);

/* lval constructors */
lval_t *lval_err(char *, ...);
lval_t *lval_num(double);
lval_t *lval_sym(char *);
lval_t *lval_fun(lbuiltin);
lval_t *lval_sexpr(void);
lval_t *lval_qexpr(void);

/* lval transformers */
lval_t *lval_add(lval_t *, lval_t *);
lval_t *lval_offer(lval_t *, lval_t *);
lval_t *lval_join(lval_t *, lval_t *);
lval_t *lval_pop(lval_t *, int);
lval_t *lval_take(lval_t *, int); /* same as pop except frees input lval */
lval_t *lval_copy(lval_t *);

char *ltype_name(ltype);

#endif


