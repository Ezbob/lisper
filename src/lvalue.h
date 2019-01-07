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

struct lvalue; 
struct lenvironment;

struct lcells {
    size_t count;
    struct lvalue **cells;
};

struct lfunction {
    struct lenvironment *env;
    struct lvalue *formals;
    struct lvalue *body;
};

struct lfile {
    struct lvalue *path;
    struct lvalue *mode;
    FILE *fp;
};

struct lvalue {
    enum ltype type;
    union val {
        double floatval;
        long long intval;
        char *strval;
        struct lcells l;
        struct lvalue *(*builtin)(struct lenvironment *, struct lvalue *);
        struct lfunction *fun;
        struct lfile *file;
    } val;
};

struct lvalue *lvalue_read(mpc_ast_t *);

void lvalue_print(struct lvalue *);
void lvalue_println(struct lvalue *);
void lvalue_del(struct lvalue *);

/* lvalue constructors */
struct lvalue *lvalue_err(char *, ...);
struct lvalue *lvalue_float(double);
struct lvalue *lvalue_bool(long long);
struct lvalue *lvalue_int(long long);
struct lvalue *lvalue_sym(char *);
struct lvalue *lvalue_str(char *);
struct lvalue *lvalue_builtin(struct lvalue *(*)(struct lenvironment *, struct lvalue *));
struct lvalue *lvalue_sexpr(void);
struct lvalue *lvalue_qexpr(void);
struct lvalue *lvalue_lambda(struct lvalue *, struct lvalue *, size_t);
struct lvalue *lvalue_file(struct lvalue *, struct lvalue *, FILE *);

/* lvalue transformers */
struct lvalue *lvalue_add(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_offer(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_join(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_join_str(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_pop(struct lvalue *, int);
struct lvalue *lvalue_take(struct lvalue *, int); /* same as pop except frees input lvalue */
struct lvalue *lvalue_copy(struct lvalue *);
int lvalue_eq(struct lvalue *, struct lvalue *);
struct lvalue *lvalue_call(struct lenvironment *, struct lvalue *, struct lvalue *);
struct lvalue *lvalue_eval(struct lenvironment *, struct lvalue *);

char *ltype_name(enum ltype);
void lvalue_pretty_print(struct lvalue *);

#endif

