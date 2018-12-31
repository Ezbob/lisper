#ifndef LISPER_EVAL
#define LISPER_EVAL

#include "mpc.h"
#include <stdlib.h>

enum ltype {
    LVAL_INT,
    LVAL_FLOAT,
    LVAL_ERR,
    LVAL_SYM,
    LVAL_SEXPR,
    LVAL_QEXPR,
    LVAL_BUILTIN,
    LVAL_LAMBDA,
    LVAL_FILE,
    LVAL_BOOL,
    LVAL_STR
};

struct lval_t; 
struct lenv_t;

typedef struct lval_t lval_t;
typedef struct lenv_t lenv_t;

typedef struct lval_t *(*lbuiltin)(struct lenv_t *, struct lval_t *);

struct lcells_t {
    size_t count;
    struct lval_t **cells;
};

struct lfunc_t {
    lenv_t *env;
    lval_t *formals;
    lval_t *body;
};

struct lfile_t {
    lval_t *path;
    lval_t *mode;
    FILE *fp;
};

struct lval_t {
    enum ltype type;
    union val {
        double floatval;
        long long intval;
        char *strval;
        struct lcells_t l;
        lbuiltin builtin;
        struct lfunc_t *fun;
        struct lfile_t *file;
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
lval_t *lval_lambda(lval_t *, lval_t *, size_t);
lval_t *lval_file(lval_t *, lval_t *, FILE *);

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

char *ltype_name(enum ltype);
void lval_pretty_print(lval_t *);

#endif

