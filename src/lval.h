#ifndef LISPER_EVAL
#define LISPER_EVAL

#include "mpc.h"
#include <stdlib.h>

typedef enum ltype {
    LVAL_INT,
    LVAL_FLOAT,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_BUILTIN,
    LVAL_LAMBDA,
    LVAL_BOOL,
    LVAL_STR
} ltype;

struct lval_t; 
struct lenv_t;

typedef struct lval_t lval_t;
typedef struct lenv_t lenv_t;

typedef lval_t *(*lbuiltin)(lenv_t *, lval_t *);

typedef struct lcells_t {
    size_t count;
    struct lval_t **cells;
} lcells_t;

typedef struct lfunc_t {
    lenv_t *env;
    lval_t *formals;
    lval_t *body;
} lfunc_t;

struct lval_t {
    ltype type;
    union val {
        double floatval;
        long long intval;
        char *err;
        char *sym;
        char *str;
        lcells_t l;
        lbuiltin builtin;
        lfunc_t *fun;
    } val;
};

lval_t *lval_read(mpc_ast_t *);

void lval_print(lval_t *);
void lval_println(lval_t *);
void lval_del(lval_t *);

/* lval constructors */
lval_t *lval_err(char *, ...);
lval_t *lval_float(double);
lval_t *lval_bool(long long);
lval_t *lval_int(long long);
lval_t *lval_sym(char *);
lval_t *lval_str(char *);
lval_t *lval_builtin(lbuiltin);
lval_t *lval_sexpr(void);
lval_t *lval_qexpr(void);
lval_t *lval_lambda(lval_t *, lval_t *);

/* lval transformers */
lval_t *lval_add(lval_t *, lval_t *);
lval_t *lval_offer(lval_t *, lval_t *);
lval_t *lval_join(lval_t *, lval_t *);
lval_t *lval_join_str(lval_t *, lval_t *);
lval_t *lval_pop(lval_t *, int);
lval_t *lval_take(lval_t *, int); /* same as pop except frees input lval */
lval_t *lval_copy(lval_t *);
int lval_eq(lval_t *, lval_t *);
lval_t *lval_call(lenv_t *, lval_t *, lval_t *);
lval_t *lval_eval(lenv_t *, lval_t *);

char *ltype_name(ltype);
void lval_pretty_print(lval_t *);

#endif

