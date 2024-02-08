#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "value.h"
#include "environment.h"
#include "mempool.h"

extern struct mempool *lvalue_mp;

struct lvalue *builtin_list(struct lenvironment *, struct lvalue *);
struct lvalue *builtin_eval(struct lenvironment *, struct lvalue *);

struct lvalue *lvalue_int(long long num) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_INT;
    val->val.intval = num;
    return val;
}

struct lvalue *lvalue_float(double num) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_FLOAT;
    val->val.floatval = num;
    return val;
}

struct lvalue *lvalue_bool(long long num) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_BOOL;
    val->val.intval = num;
    return val;
}

struct lvalue *lvalue_err(char *fmt, ...) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_ERR;
    va_list va;
    va_start(va, fmt);

    val->val.strval = malloc(512 * sizeof(char));
    vsnprintf(val->val.strval, 511, fmt, va);
    val->val.strval = realloc(val->val.strval, strlen(val->val.strval) + 1);

    va_end(va);
    return val;
}

struct lvalue *lvalue_sym(char* sym) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_SYM;
    val->val.strval = malloc(strlen(sym) + 1);
    strcpy(val->val.strval, sym);
    return val;
}

struct lvalue *lvalue_sexpr(void) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_SEXPR;
    val->val.l.count = 0;
    val->val.l.cells = NULL;
    return val;
}

struct lvalue *lvalue_qexpr(void) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_QEXPR;
    val->val.l.count = 0;
    val->val.l.cells = NULL;
    return val;
}

struct lvalue *lvalue_str(char *s) {
    struct lvalue *v = mempool_take(lvalue_mp);
    v->type = LVAL_STR;
    v->val.strval = malloc(strlen(s) + 1);
    strcpy(v->val.strval, s);
    return v;
}

struct lvalue *lvalue_builtin(struct lvalue *( *f)(struct lenvironment *, struct lvalue *)) {
    struct lvalue *val = mempool_take(lvalue_mp);
    val->type = LVAL_BUILTIN;
    val->val.builtin = f;
    return val;
}

struct lfunction *lfunc_new(struct lenvironment *env, struct lvalue *formals, struct lvalue *body) {
    struct lfunction *new = malloc(sizeof(struct lfunction));
    new->name = NULL;
    new->env = env;
    new->formals = formals;
    new->body = body;
    return new;
}

struct lfile *lfile_new(struct lvalue *path, struct lvalue *mode, FILE *fp) {
    struct lfile *new = malloc(sizeof(struct lfile));
    new->path = path;
    new->mode = mode;
    new->fp = fp;
    return new;
}

struct lvalue *lvalue_lambda(struct lvalue *formals, struct lvalue *body, size_t envcap) {
    struct lvalue *nw = mempool_take(lvalue_mp);
    nw->type = LVAL_FUNCTION;
    nw->val.fun = lfunc_new(lenvironment_new(envcap), formals, body);
    return nw;
}

struct lvalue *lvalue_file(struct lvalue *path, struct lvalue *mode, FILE *fp) {
    struct lvalue *nw = mempool_take(lvalue_mp);
    nw->type = LVAL_FILE;
    nw->val.file = lfile_new(path, mode, fp);
    return nw;
}


void lvalue_del(struct lvalue *val) {
    struct lfile *file;
    struct lfunction *func;
    switch (val->type) {
        case LVAL_FLOAT:
        case LVAL_INT:
        case LVAL_BUILTIN:
        case LVAL_BOOL:
            break;
        case LVAL_FUNCTION:
            func = val->val.fun;
            lenvironment_del(func->env);
            lvalue_del(func->formals);
            lvalue_del(func->body);
            free(func);
            break;
        case LVAL_FILE:
            file = val->val.file;
            lvalue_del(file->path);
            lvalue_del(file->mode);
            free(file);
            break;
        case LVAL_ERR:
        case LVAL_SYM:
        case LVAL_STR:
            free(val->val.strval);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for ( size_t i = 0; i < val->val.l.count; ++i ) {
                lvalue_del(val->val.l.cells[i]);
            }
            free(val->val.l.cells);
            break;
    }
    mempool_recycle(lvalue_mp, val);
}


/**
 * Prints lvalue expressions (such as sexprs) given the prefix,
 * suffix and delimiter
 */
void lvalue_expr_print(struct lvalue *val, char prefix, char suffix, char delimiter) {
    putchar(prefix);

    size_t len = val->val.l.count;

    if ( len > 0 ) lvalue_print(val->val.l.cells[0]);

    for ( size_t i = 1; i < len; i++ ) {
        putchar(delimiter);
        lvalue_print(val->val.l.cells[i]);
    }

    putchar(suffix);
}

/**
 * Escapes the string value of the input lvalue
 * and prints the value to the stdout
 */
void lvalue_print_str(struct lvalue *v) {
    char *escaped = malloc(strlen(v->val.strval) + 1);
    strcpy(escaped, v->val.strval);

    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);

    free(escaped);
}

/**
 * Prints the lvalue contents to stdout
 */
void lvalue_print(struct lvalue *val) {
    switch ( val->type ) {
        case LVAL_FLOAT:
            printf("%lf", val->val.floatval);
            break;
        case LVAL_INT:
            printf("%lli", val->val.intval);
            break;
        case LVAL_BOOL:
            if ( val->val.intval == 0 ) {
                printf("false");
            } else {
                printf("true");
            }
            break;
        case LVAL_ERR:
            printf("Error: %s", val->val.strval);
            break;
        case LVAL_SYM:
            printf("%s", val->val.strval);
            break;
        case LVAL_STR:
            lvalue_print_str(val);
            break;
        case LVAL_SEXPR:
            lvalue_expr_print(val, '(', ')', ' ');
            break;
        case LVAL_QEXPR:
            lvalue_expr_print(val, '{', '}', ' ');
            break;
        case LVAL_FUNCTION:
            printf("(\\ ");
            lvalue_print(val->val.fun->formals);
            putchar(' ');
            lvalue_print(val->val.fun->body);
            putchar(')');
            break;
        case LVAL_BUILTIN:
            printf("<builtin>");
            break;
        case LVAL_FILE:
            printf("<file ");
            lvalue_print(val->val.file->path);
            printf(" @ mode ");
            lvalue_print(val->val.file->mode);
            printf(">");
            break;
    }
}

void lvalue_println(struct lvalue *val) {
    lvalue_print(val);
    putchar('\n');
}

enum {
    LREAD_FLOAT = 0,
    LREAD_INT = 1
};

struct lvalue *lvalue_read_num(mpc_ast_t *t, int choice) {

    int code = 0;
    long long int int_read = 0;
    double float_read = 0.0;

    switch (choice) {
        case LREAD_FLOAT:
            code = sscanf(t->contents, "%lf", &float_read);

            if ( code == 1 ) {
                return lvalue_float(float_read);
            }
            break;
        case LREAD_INT:
            code = sscanf(t->contents, "%lli", &int_read);

            if ( code == 1 ) {
                return lvalue_int(int_read);
            }
            break;
    }

    return lvalue_err("Cloud not parse '%s' as a number.", t->contents);
}

struct lvalue *lvalue_add(struct lvalue *val, struct lvalue *other) {
    val->val.l.count++;
    struct lvalue **resized_cells = realloc(val->val.l.cells, val->val.l.count * sizeof(struct lvalue *));
    if ( resized_cells == NULL ) {
        lvalue_del(val);
        lvalue_del(other);
        perror("Could not resize lvalue cell buffer");
        exit(1);
    }
    resized_cells[val->val.l.count - 1] = other;
    val->val.l.cells = resized_cells;
    return val;
}

struct lvalue *lvalue_offer(struct lvalue *val, struct lvalue *other) {
    val->val.l.count++;
    struct lvalue **resized = realloc(val->val.l.cells, val->val.l.count * sizeof(struct lvalue*));
        // resize the memory buffer to carry another cell

    if ( resized == NULL ) {
        perror("Fatal memory error when trying reallocating for offer");
        lvalue_del(val);
        exit(1);
    }
    val->val.l.cells = resized;
    memmove(val->val.l.cells + 1, val->val.l.cells, (val->val.l.count - 1) * sizeof(struct lvalue*));
        // move memory at address val->val.l.cells (up to old count of cells) to addr val->val.l.cells[1]

    val->val.l.cells[0] = other;
        // insert into the front of the array

    return val;
}

struct lvalue *lvalue_join_str(struct lvalue *x, struct lvalue *y) {
    size_t cpy_start = strlen(x->val.strval);
    size_t total_size = cpy_start + strlen(y->val.strval);

    char *resized = realloc(x->val.strval, total_size + 1);
    if ( resized == NULL ) {
        perror("Fatal memory error when trying reallocating for join_str");
        lvalue_del(x);
        lvalue_del(y);
        exit(1);
    }
    x->val.strval = resized;
    strcpy(x->val.strval + cpy_start, y->val.strval);

    lvalue_del(y);
    return x;
}

struct lvalue *lvalue_join(struct lvalue *x, struct lvalue *y) {
    while ( y->val.l.count ) {
        x = lvalue_add(x, lvalue_pop(y, 0));
    }

    lvalue_del(y);
    return x;
}

struct lvalue *lvalue_read_str(mpc_ast_t *t) {

    t->contents[strlen(t->contents) - 1] = '\0';
        /* clip off the newline */

    char *unescaped = malloc(strlen(t->contents + 1) + 1);
    strcpy(unescaped, t->contents + 1);
        /*  but make room for the newline in the unescaped output */

    unescaped = mpcf_unescape(unescaped);
        /* unescape probably inserts a newline into the string */
    struct lvalue *str = lvalue_str(unescaped);

    free(unescaped);
    return str;
}

struct lvalue *lvalue_read(mpc_ast_t *t) {
    struct lvalue *val = NULL;

    if ( strstr(t->tag, "boolean") ) {
        long long res = (strcmp(t->contents, "true") == 0) ? 1 : 0;
        return lvalue_bool(res);
    }

    if ( strstr(t->tag, "string") ) {
        return lvalue_read_str(t);
    }

    if ( strstr(t->tag, "float") ) {
        return lvalue_read_num(t, LREAD_FLOAT);
    }

    if ( strstr(t->tag, "integer") ) {
        return lvalue_read_num(t, LREAD_INT);
    }

    if ( strstr(t->tag, "symbol") ) {
        return lvalue_sym(t->contents);
    }

    if ( strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr") ) {
        /* ">" is the root of the AST */
        val = lvalue_sexpr();
    }

    if ( strstr(t->tag, "qexpr") ) {
        val = lvalue_qexpr();
    }

    for ( int i = 0; i < t->children_num; i++ ) {
        struct mpc_ast_t *child = t->children[i];
        if ( strcmp(child->contents, "(") == 0 ||
             strcmp(child->contents, ")") == 0 ||
             strcmp(child->contents, "{") == 0 ||
             strcmp(child->contents, "}") == 0 ||
             strcmp(child->tag, "regex") == 0  ||
             strstr(child->tag, "comment") ) {
            continue;
        }
        val = lvalue_add(val, lvalue_read(child));
    }

    return val;
}

/**
 * Pops the value of the input lvalue at index i,
 * and resizes the memory buffer
 */
struct lvalue *lvalue_pop(struct lvalue *v, int i) {
    struct lvalue *x = v->val.l.cells[i];
    memmove(v->val.l.cells + i, v->val.l.cells + (i + 1), sizeof(struct lvalue *) * (v->val.l.count - i - 1));
    v->val.l.count--;

    struct lvalue **cs = realloc(v->val.l.cells, v->val.l.count * sizeof(struct lvalue *));
    if ( !cs && v->val.l.count > 0 ) {
        perror("Could not shrink cell buffer");
        lvalue_del(v);
        exit(1);
    }
    v->val.l.cells = cs;

    return x;
}

/**
 * Pop the value off the input value at index i, and
 * delete the input value
 */
struct lvalue *lvalue_take(struct lvalue *v, int i) {
    struct lvalue *x = lvalue_pop(v, i);
    lvalue_del(v);
    return x;
}

/**
 * Create a copy of the input lvalue
 */
struct lvalue *lvalue_copy(struct lvalue *v) {
    struct lvalue *x = mempool_take(lvalue_mp);
    x->type = v->type;
    struct lvalue *p;
    FILE *fp;
    struct lvalue *m;

    switch(v->type) {
        case LVAL_FUNCTION:
            x->val.fun = lfunc_new(
                lenvironment_copy(v->val.fun->env),
                lvalue_copy(v->val.fun->formals),
                lvalue_copy(v->val.fun->body)
            );
            break;
        case LVAL_BUILTIN:
            x->val.builtin = v->val.builtin;
            break;
        case LVAL_FLOAT:
            x->val.floatval = v->val.floatval;
            break;
        case LVAL_ERR:
        case LVAL_SYM:
        case LVAL_STR:
            x->val.strval = malloc((strlen(v->val.strval) + 1) * sizeof(char));
            strcpy(x->val.strval, v->val.strval);
            break;
        case LVAL_INT:
        case LVAL_BOOL:
            x->val.intval = v->val.intval;
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->val.l.count = v->val.l.count;
            x->val.l.cells = malloc(v->val.l.count * sizeof(struct lvalue *));
            for ( size_t i = 0; i < x->val.l.count; ++i ) {
                x->val.l.cells[i] = lvalue_copy(v->val.l.cells[i]);
            }
            break;
        case LVAL_FILE:
            p = lvalue_copy(v->val.file->path);
            m = lvalue_copy(v->val.file->mode);
            fp = v->val.file->fp; /* copy share fp to limit fp use to the same file */
            x->val.file = lfile_new(p, m, fp);
            break;
     }

    return x;
}

/**
 * Compare two lvalues for equality.
 * Returns zero if input values x and y are not equal,
 * returns a non-zero otherwise
 */
int lvalue_eq(struct lvalue *x, struct lvalue *y) {
    if ( x->type != y->type ) {
        return 0;
    }

    switch ( x->type ) {
        case LVAL_FLOAT:
            return (x->val.floatval == y->val.floatval);
        case LVAL_BOOL:
        case LVAL_INT:
            return (x->val.intval == y->val.intval);
        case LVAL_ERR:
        case LVAL_SYM:
        case LVAL_STR:
            return strcmp(x->val.strval, y->val.strval) == 0;
        case LVAL_BUILTIN:
            return (x->val.builtin == y->val.builtin);
        case LVAL_FUNCTION:
            return lvalue_eq(x->val.fun->formals, y->val.fun->formals) &&
                 lvalue_eq(x->val.fun->body, y->val.fun->body);
        case LVAL_FILE:
            return lvalue_eq(x->val.file->path, y->val.file->path) &&
                lvalue_eq(x->val.file->mode, y->val.file->mode) &&
                x->val.file->fp == y->val.file->fp;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if ( x->val.l.count != y->val.l.count ) {
                return 0;
            }
            for ( size_t i = 0; i < x->val.l.count; ++i ) {
                if ( !lvalue_eq(x->val.l.cells[i], y->val.l.cells[i]) ) {
                    return 0;
                }
            }
            return 1;
    }
    return 0;
}

/**
 * Given a lvalue type, return it's string representation
 */
char *ltype_name(enum ltype t) {
    switch ( t ) {
        case LVAL_SYM:
            return "symbol";
        case LVAL_ERR:
            return "error";
        case LVAL_STR:
            return "string";
        case LVAL_BUILTIN:
            return "builtin";
        case LVAL_FUNCTION:
            return "function";
        case LVAL_FLOAT:
            return "float";
        case LVAL_BOOL:
            return "boolean";
        case LVAL_INT:
            return "integer";
        case LVAL_QEXPR:
            return "q-expression";
        case LVAL_SEXPR:
            return "s-expression";
        default:
            break;
    }
    return "Unknown";
}

/**
 * Helper function for printing lvalue
 */
void lvalue_depth_print(struct lvalue *v, size_t depth) {

    for ( size_t i = 0; i < depth; ++i ) {
        putchar(' ');
    }

    printf("`-t: %s = ", ltype_name(v->type));

    lvalue_println(v);

    /* only q-expressions and s-expression has children */
    if ( v->type == LVAL_QEXPR || v->type == LVAL_SEXPR ) {
        for ( size_t i = 0; i < v->val.l.count; ++i ) {
            lvalue_depth_print(v->val.l.cells[i], depth + 1);
        }
    }
}

/**
 *  Pretty print a lvalue revealing it's structure.
 *  Good for debugging your thoughts
 */
void lvalue_pretty_print(struct lvalue *v) {
    lvalue_depth_print(v, 0);
}

/**
 * evaluate a function
 */
struct lvalue *lvalue_call(struct lenvironment *e, struct lvalue *f, struct lvalue *v) {

    if ( f->type == LVAL_BUILTIN ) {
        return f->val.builtin(e, v);
    }

    struct lfunction *func = f->val.fun;
    struct lvalue **args = v->val.l.cells;
    struct lvalue *formals = func->formals;

    size_t given = v->val.l.count;
    size_t total = formals->val.l.count;

    while ( v->val.l.count > 0 ) {

        if ( formals->val.l.count == 0 &&
            args[0]->type != LVAL_SEXPR ) {
            /* Error case: non-symbolic parameter parsed */
            lvalue_del(v);
            return lvalue_err(
                "Function parsed too many arguments; "
                "got %lu expected %lu",
                given,
                total
            );
        } else if ( formals->val.l.count == 0 &&
            args[0]->type == LVAL_SEXPR &&
            args[0]->val.l.count == 0 ) {
            /* Function with no parameters case: can be called with a empty sexpr */
            break;
        }

        struct lvalue *sym = lvalue_pop(formals, 0); /* unbound name */

        if ( strcmp(sym->val.strval, "&") == 0 ) {
            /* Variable argument case with '&' */
            if ( formals->val.l.count != 1 ) {
                lvalue_del(v);
                return lvalue_err(
                    "Function format invalid. "
                    "Symbol '&' not followed by a single symbol."
                );
            }

            /* Binding rest of the arguments to nsym */
            struct lvalue *nsym = lvalue_pop(formals, 0);
            lenvironment_put(func->env, nsym, builtin_list(e, v));
            lvalue_del(sym);
            lvalue_del(nsym);
            break;
        }

        struct lvalue *val = lvalue_pop(v, 0); /* value to apply to unbound name */

        lenvironment_put(func->env, sym, val);
        lvalue_del(sym);
        lvalue_del(val);
    }

    lvalue_del(v);

    if ( formals->val.l.count > 0 &&
        strcmp(formals->val.l.cells[0]->val.strval, "&") == 0 ) {
        /* only first non-variable arguments was applied; create a empty qexpr  */

        if ( formals->val.l.count != 2 ) {
            return lvalue_err(
                "Function format invalid. "
                "Symbol '&' not followed by single symbol."
            );
        }

        /* Remove '&' */
        lvalue_del(lvalue_pop(formals, 0));

        /* Create a empty list for the next symbol */
        struct lvalue *sym = lvalue_pop(formals, 0);
        struct lvalue *val = lvalue_qexpr();

        lenvironment_put(func->env, sym, val);
        lvalue_del(sym);
        lvalue_del(val);
    }

    if ( formals->val.l.count == 0 ) {
        func->env->parent = e;
        return builtin_eval( func->env, lvalue_add(lvalue_sexpr(), lvalue_copy(func->body)) );
    }
    return lvalue_copy(f);
}


struct lvalue *lvalue_eval_sexpr(struct lenvironment *e, struct lvalue *v) {
    /* empty sexpr */
    if ( v->val.l.count == 0 ) {
        return v;
    }

    /*
     * Implicit qouting
     * Intercept special sexpr to make symbols qouted
     */
    struct lvalue *first = v->val.l.cells[0];
    if ( first->type == LVAL_SYM && v->val.l.count > 1 ) {
        struct lvalue *sec = v->val.l.cells[1];
        if (
            (
                strcmp(first->val.strval, "=") == 0    ||
                strcmp(first->val.strval, "def") == 0  ||
                strcmp(first->val.strval, "fn") == 0
            ) && sec->type == LVAL_SYM
        ) {
            v->val.l.cells[1] = lvalue_add(lvalue_qexpr(), sec);
        }
    }

    /* Depth-first evaluation of sexpr arguments.
       This resolves the actual meaning of the sexpr, such that
       what operator to apply to this sexpr is known, and if
       there was any error executing nested sexpr etc...
     */
    for ( size_t i = 0; i < v->val.l.count; i++ ) {
        v->val.l.cells[i] = lvalue_eval(e, v->val.l.cells[i]);
    }

    /* Hoist first lvalue if only one is available.
       This helps to sub results of sexprs, but
       the sub lval.has to be evaluated first.
     */
    if ( v->val.l.count == 1 ) {
        return lvalue_take(v, 0);
    }

    /* Return first error (if any) */
    for ( size_t i = 0; i < v->val.l.count; i++ ) {
        if ( v->val.l.cells[i]->type == LVAL_ERR ) {
            return lvalue_take(v, i);
        }
    }

    /* Function evaluation.
       Take the first lvalue in the sexpression and apply it to the
       following lvalue sequence
    */
    struct lvalue *operator = lvalue_pop(v, 0);
    struct lvalue *res = NULL;
    char *type_name = NULL;

    switch ( operator->type ) {
        case LVAL_FUNCTION:
        case LVAL_BUILTIN:
            res = lvalue_call(e, operator, v);
            break;
        default:
            type_name = ltype_name(operator->type);
            lvalue_del(operator);
            lvalue_del(v);
            return lvalue_err("Expected first argument of %s to be of type '%s'; got '%s'.",
                        ltype_name(LVAL_SEXPR), ltype_name(LVAL_BUILTIN), type_name);
    }

    lvalue_del(operator);
    return res;
}

struct lvalue *lvalue_eval(struct lenvironment *e, struct lvalue *v) {

    struct lvalue *x;
    switch ( v->type ) {
        case LVAL_SYM:
            /* Eval'ing symbols just looks up the symbol in the symbol table
               Discards the wrapping.
             */
            x = lenvironment_get(e, v);
            lvalue_del(v);
            return x;

        case LVAL_SEXPR:
            /* this might by a nested sexpr or toplevel */
            return lvalue_eval_sexpr(e, v);
        default:
            break;
    }

    return v;
}

