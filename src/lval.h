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

typedef struct lval_t *(*lbuiltin)(struct lenv_t *, struct lval_t *);

struct lcells_t {
    size_t count;
    struct lval_t **cells;
};

struct lfunc_t {
    struct lenv_t *env;
    struct lval_t *formals;
    struct lval_t *body;
};

struct lfile_t {
    struct lval_t *path;
    struct lval_t *mode;
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

struct lval_t *lval_read(mpc_ast_t *);

void lval_print(struct lval_t *);
void lval_println(struct lval_t *);
void lval_del(struct lval_t *);

/* lval constructors */
struct lval_t *lval_err(char *, ...);
struct lval_t *lval_float(double);
struct lval_t *lval_bool(long long);
struct lval_t *lval_int(long long);
struct lval_t *lval_sym(char *);
struct lval_t *lval_str(char *);
struct lval_t *lval_builtin(lbuiltin);
struct lval_t *lval_sexpr(void);
struct lval_t *lval_qexpr(void);
struct lval_t *lval_lambda(struct lval_t *, struct lval_t *, size_t);
struct lval_t *lval_file(struct lval_t *, struct lval_t *, FILE *);

/* lval transformers */
struct lval_t *lval_add(struct lval_t *, struct lval_t *);
struct lval_t *lval_offer(struct lval_t *, struct lval_t *);
struct lval_t *lval_join(struct lval_t *, struct lval_t *);
struct lval_t *lval_join_str(struct lval_t *, struct lval_t *);
struct lval_t *lval_pop(struct lval_t *, int);
struct lval_t *lval_take(struct lval_t *, int); /* same as pop except frees input lval */
struct lval_t *lval_copy(struct lval_t *);
int lval_eq(struct lval_t *, struct lval_t *);
struct lval_t *lval_call(struct lenv_t *, struct lval_t *, struct lval_t *);
struct lval_t *lval_eval(struct lenv_t *, struct lval_t *);

char *ltype_name(enum ltype);
void lval_pretty_print(struct lval_t *);

#endif

