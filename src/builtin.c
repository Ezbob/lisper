#include <stdlib.h>
#include <math.h>
#include "lisper.h"
#include "builtin.h"
#include "grammar.h"
#include "lval.h"
#include "lenv.h"

#define LGETCELL(v, celln) v->val.l.cells[celln]

#define UNUSED(x) (void)(x)

#define LIS_NUM(type) (type == LVAL_INT || type == LVAL_FLOAT)

#define LASSERT(lvalue, cond, fmt, ...) \
    if ( !(cond) ) { struct lval_t *err = lval_err(fmt, ##__VA_ARGS__); lval_del(lvalue); return err;  }

#define LNUM_LEAST_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l.count >= numargs, "Wrong number of arguments parsed to '%s'. Expected at least %lu argument(s); got %lu. ", funcname, numargs, sym->val.l.count)

#define LNUM_ARGS(lvalue, func_name, numargs) \
    LASSERT(lvalue, lvalue->val.l.count == numargs, "Wrong number of arguments parsed to '%s'. Expected at exactly %lu argument(s); got %lu. ", func_name, numargs, lvalue->val.l.count)

#define LNOT_EMPTY_QEXPR(lvalue, func_name, i) \
    LASSERT(lvalue, lvalue->val.l.cells[i]->type == LVAL_QEXPR && lvalue->val.l.cells[i]->val.l.count > 0, "Empty %s parsed to '%s'.", ltype_name(lvalue->val.l.cells[i]->type, func_name))

#define LARG_TYPE(lvalue, func_name, i, expected) \
    LASSERT(lvalue, lvalue->val.l.cells[i]->type == expected, "Wrong type of argument parsed to '%s' at argument position %lu. Expected argument to be of type '%s'; got '%s'.", func_name, (i + 1), ltype_name(expected), ltype_name(lvalue->val.l.cells[i]->type))

#define LTWO_ARG_TYPES(lvalue, func_name, i, first_expect, second_expect) \
    LASSERT(lvalue, lvalue->val.l.cells[i]->type == first_expect || lvalue->val.l.cells[i]->type == second_expect, "Wrong type of argument parsed to '%s'. Expected argument to be of type '%s' or '%s'; got '%s'.", func_name, ltype_name(first_expect), ltype_name(second_expect), ltype_name(lvalue->val.l.cells[i]->type))

#define LENV_BUILTIN(name) lenv_add_builtin(e, #name, builtin_##name)
#define LENV_SYMBUILTIN(sym, name) lenv_add_builtin(e, sym, builtin_##name)

#define LMATH_TYPE_CHECK(lvalue, sym) do { \
    enum ltype expected_arg_type = LGETCELL(lvalue, 0)->type; \
    LASSERT(lvalue, LIS_NUM(expected_arg_type), "Cannot operate on argument at position %i. Non-number type '%s' parsed to operator '%s'.", 1, ltype_name(expected_arg_type), sym); \
    for ( size_t i = 1; i < lvalue->val.l.count; i++ ) { \
        struct lval_t *curr = LGETCELL(lvalue, i); \
        LASSERT(lvalue, expected_arg_type == curr->type, "Argument type mismatch. Expected argument at position %lu to be of type '%s'; got type '%s'.", i + 1, ltype_name(expected_arg_type), ltype_name(curr->type)); \
    } \
} while (0)

extern grammar_elems elems;
extern struct argument_capture *args;

/* env preallocated sizes */
const size_t lambda_env_prealloc = 50;
const size_t fun_env_prealloc = 200;

/* * math builtins * */

struct lval_t *builtin_add(struct lenv_t *e, struct lval_t *a) {
    UNUSED(e);
    LMATH_TYPE_CHECK(a, "+");
    struct lval_t *res = lval_pop(a, 0);

    if ( res->type == LVAL_INT ) {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            res->val.intval += b->val.intval;
            lval_del(b);
        }
    } else {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            res->val.floatval += b->val.floatval;
            lval_del(b);
        }
    }

    lval_del(a);
    return res;
}

struct lval_t *builtin_sub(struct lenv_t *e, struct lval_t *a) {
    UNUSED(e);
    LMATH_TYPE_CHECK(a, "-");
    struct lval_t *res = lval_pop(a, 0);

    if ( res->type == LVAL_INT ) {
        if ( a->val.l.count == 0 ) {
            res->val.intval = (-res->val.intval);
        } else {
            while ( a->val.l.count > 0 ) {
                struct lval_t *b = lval_pop(a, 0);
                res->val.intval -= b->val.intval;
                lval_del(b);
            }
        }
    } else {
        if ( a->val.l.count == 0 ) {
            res->val.floatval = (-res->val.floatval);
        } else {
            while ( a->val.l.count > 0 ) {
                struct lval_t *b = lval_pop(a, 0);
                res->val.floatval -= b->val.floatval;
                lval_del(b);
            }
        }
    }

    lval_del(a);
    return res;
}

struct lval_t *builtin_mul(struct lenv_t *e, struct lval_t *a) {
    UNUSED(e);
    LMATH_TYPE_CHECK(a, "*");
    struct lval_t *res = lval_pop(a, 0);

    if ( res->type == LVAL_INT ) {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            res->val.intval *= b->val.intval;
            lval_del(b);
        }
    } else {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            res->val.floatval *= b->val.floatval;
            lval_del(b);
        }
    }

    lval_del(a);
    return res;
}

struct lval_t *builtin_div(struct lenv_t *e, struct lval_t *a) {
    UNUSED(e);
    LMATH_TYPE_CHECK(a, "/");
    struct lval_t *res = lval_pop(a, 0);

    if ( res->type == LVAL_INT ) {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( b->val.intval == 0 ) {
                lval_del(res);
                lval_del(b);
                res = lval_err("Division by zero");
                break;
            }
            res->val.intval /= b->val.intval;
            lval_del(b);
        }
    } else {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( b->val.floatval == 0 ) {
                lval_del(res);
                lval_del(b);
                res = lval_err("Division by zero");
                break;
            }
            res->val.floatval /= b->val.floatval;
            lval_del(b);
        }
    }

    lval_del(a);
    return res;
}


struct lval_t *builtin_mod(struct lenv_t *e, struct lval_t *a) {
    UNUSED(e);
    LMATH_TYPE_CHECK(a, "%");
    struct lval_t *res = lval_pop(a, 0);

    if ( res->type == LVAL_INT ) {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( b->val.intval == 0 ) {
                lval_del(res);
                lval_del(b);
                res = lval_err("Division by zero");
                break;
            }
            res->val.intval %= b->val.intval;
            lval_del(b);
        }
    } else {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( b->val.floatval == 0 ) {
                lval_del(res);
                lval_del(b);
                res = lval_err("Division by zero");
                break;
            }
            res->val.floatval = fmod(res->val.floatval, b->val.floatval);
            lval_del(b);
        }
    }

    lval_del(a);
    return res;
}

struct lval_t *builtin_min(struct lenv_t *e, struct lval_t *a) {
    UNUSED(e);
    LMATH_TYPE_CHECK(a, "min");
    struct lval_t *res = lval_pop(a, 0);

    if ( res->type == LVAL_INT ) {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( res->val.intval > b->val.intval ) {
                res->val.intval = b->val.intval;
            }
            lval_del(b);
        }
    } else {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( res->val.floatval > b->val.floatval ) {
                res->val.floatval = b->val.floatval;
            }
            lval_del(b);
        }
    }

    lval_del(a);
    return res;
}

struct lval_t *builtin_max(struct lenv_t *e, struct lval_t *a) {
    UNUSED(e);
    LMATH_TYPE_CHECK(a, "max");
    struct lval_t *res = lval_pop(a, 0);

    if ( res->type == LVAL_INT ) {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( res->val.intval < b->val.intval ) {
                res->val.intval = b->val.intval;
            }
            lval_del(b);
        }
    } else {
        while ( a->val.l.count > 0 ) {
            struct lval_t *b = lval_pop(a, 0);
            if ( res->val.floatval < b->val.floatval ) {
                res->val.floatval = b->val.floatval;
            }
            lval_del(b);
        }
    }

    lval_del(a);
    return res;
}

/* * q-expression specific builtins * */

/**
 * Convert expression into a q-expression
 */
struct lval_t *builtin_list(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    v->type = LVAL_QEXPR;
    return v;
}

/**
 * Take one q-expression and evaluate it
 * as a s-expression.
 */
struct lval_t *builtin_eval(struct lenv_t *e, struct lval_t *v) {
    LNUM_ARGS(v, "eval", 1);
    LARG_TYPE(v, "eval", 0, LVAL_QEXPR);

    struct lval_t *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(e, a);
}

/**
 * Read a string and try and parse into a
 * lval
 */
struct lval_t *builtin_read(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "read", 1);
    LARG_TYPE(v, "read", 0, LVAL_STR);

    mpc_result_t r;
    if ( mpc_parse("input", LGETCELL(v, 0)->val.strval, elems.Lisper, &r) ) {
        struct lval_t *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        lval_del(v);
        return builtin_list(e, expr);
    }

    /* parse error */
    char *err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);

    struct lval_t *err = lval_err("Could parse str %s", err_msg);
    free(err_msg);
    lval_del(v);

    return err;
}

/**
 * Print the contents of a string
 */
struct lval_t *builtin_show(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "show", 1);
    LARG_TYPE(v, "show", 0, LVAL_STR);

    puts(LGETCELL(v, 0)->val.strval);

    lval_del(v);
    return lval_sexpr();
}

/* * IO builtins * */

/**
 * get a list of input program arguments
 */
struct lval_t *builtin_args(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "args", 1);

    struct lval_t *res = lval_qexpr();
    struct lval_t *arg; 
    int argc = args->argc;
    for (int i = 0; i < argc; ++i) {
        char *arg_str = args->argv[i];
        arg = lval_str(arg_str);
        lval_add(res, arg);
    }

    lval_del(v);
    return res;
}


/**
 * Print a series of lvals
 */
struct lval_t *builtin_print(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);

    for ( size_t i = 0; i < v->val.l.count; ++i ) {
        lval_print(LGETCELL(v, i));
        putchar(' ');
    }

    putchar('\n');
    lval_del(v);

    return lval_sexpr();
}

/**
 * Open a file in one of the following modes:
 * - "r": read-only mode
 * - "r+": read and write mode (on pre-existing file)
 * - "w": write-only mode that overrides existing files
 * - "a": append mode
 * - "w+": read and write mode that overrides existing files
 * - "a+": read and write mode that appends to existing files
 */
struct lval_t *builtin_open(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "open", 2);
    LARG_TYPE(v, "open", 0, LVAL_STR);
    LARG_TYPE(v, "open", 1, LVAL_STR);

    struct lval_t *filename = lval_pop(v, 0);
    struct lval_t *mode = lval_pop(v, 0);

    char *m = mode->val.strval;
    char *path = filename->val.strval;
    FILE *fp;

    if ( !(strcmp(m, "r") == 0 ||
           strcmp(m, "r+") == 0 ||
           strcmp(m, "w") == 0 ||
           strcmp(m, "w+") == 0 ||
           strcmp(m, "a") == 0 ||
           strcmp(m, "a+") == 0) ) {
        struct lval_t *err = lval_err("Mode not set to either 'r', 'r+', 'w', 'w+', 'a' or 'a+'");
        lval_del(mode);
        lval_del(filename);
        lval_del(v);
        return err;
    }

    if ( strlen(path) == 0 ) {
        lval_del(mode);
        lval_del(filename);
        lval_del(v);
        return lval_err("Empty file path");
    }

    fp = fopen(path, m);

    if ( fp == NULL ) {
        lval_del(v);
        return lval_err("Could not open file '%s'. %s", path, strerror(errno));
    }

    lval_del(v);
    return lval_file(filename, mode, fp);
}

/**
 * Closes an open file
 */
struct lval_t *builtin_close(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "close", 1);
    LARG_TYPE(v, "close", 0, LVAL_FILE);

    struct lval_t *f = LGETCELL(v, 0);

    if ( fclose(f->val.file->fp) != 0 ) {
        lval_del(v);
        return lval_err("Cloud not close file: '%s'", f->val.file->path);
    }

    lval_del(v);
    return lval_sexpr();
}

struct lval_t *builtin_flush(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "flush", 1);
    LARG_TYPE(v, "flush", 0, LVAL_FILE);

    struct lval_t *f = LGETCELL(v, 0);

    if ( fflush(f->val.file->fp) != 0 ) {
        lval_del(v);
        return lval_err("Cloud not flush file buffer for: '%s'", f->val.file->path);
    }

    lval_del(v);
    return lval_sexpr();
}

struct lval_t *builtin_putstr(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "putstr", 2);
    LARG_TYPE(v, "putstr", 0, LVAL_STR);
    LARG_TYPE(v, "putstr", 1, LVAL_FILE);

    struct lval_t *f = LGETCELL(v, 1);
    struct lval_t *str = LGETCELL(v, 0);

    if ( fputs(str->val.strval, f->val.file->fp) == EOF ) {
        lval_del(v);
        return lval_err("Could write '%s' to file", str->val.strval);
    }

    return lval_sexpr();
}

struct lval_t *builtin_getstr(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "getstr", 1);
    LARG_TYPE(v, "getstr", 0, LVAL_FILE);

    struct lval_t *f = LGETCELL(v, 0);

    char *s = calloc(16385, sizeof(char));

    if ( fgets(s, 16384 * sizeof(char), f->val.file->fp) == NULL ) {
        lval_del(v);
        return lval_err("Could not get string from file; could not read string");
    }

    char *resized = realloc(s, strlen(s) + 1);
    if ( resized == NULL ) {
        lval_del(v);
        return lval_err("Could not get string from file; could not resize string buffer");
    }

    return lval_str(resized);
}

struct lval_t *builtin_rewind(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "rewind", 1);
    LARG_TYPE(v, "rewind", 0, LVAL_FILE);

    rewind(LGETCELL(v, 0)->val.file->fp);

    return lval_sexpr();
}

/* * error builtins * */

struct lval_t *builtin_error(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "error", 1);
    LARG_TYPE(v, "error", 0, LVAL_STR);

    struct lval_t *err = lval_err(LGETCELL(v, 0)->val.strval);

    lval_del(v);
    return err;
}


/* * collection builtins * */

struct lval_t *builtin_tail(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "tail", 1);
    LTWO_ARG_TYPES(v, "tail", 0, LVAL_QEXPR, LVAL_STR);

    struct lval_t *a = lval_take(v, 0);

    if ( a->type == LVAL_STR ) {
        if ( strlen(a->val.strval) > 1 ) {
            struct lval_t *tail = lval_str(a->val.strval + 1);
            lval_del(a);
            return tail;
        }
        lval_del(a);
        return lval_str("");
    } else {

        if ( a->val.l.count > 0 ) {
            lval_del(lval_pop(a, 0));
        }
        return a;
    }
}

struct lval_t *builtin_head(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "head", 1);
    LTWO_ARG_TYPES(v, "head", 0, LVAL_QEXPR, LVAL_STR);

    struct lval_t *a = lval_take(v, 0);

    if ( a->type == LVAL_STR ) {
        if ( strlen(a->val.strval) > 1 ) {
            char second = a->val.strval[1];
            a->val.strval[1] = '\0';

            struct lval_t *head = lval_str(a->val.strval);
            a->val.strval[1] = second;

            lval_del(a);
            return head;
        }
        lval_del(a);
        return lval_str("");
    } else {
        while ( a->val.l.count > 1 ) {
            lval_del(lval_pop(a, 1));
        }
        return a;
    }
}

struct lval_t *builtin_join(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    for ( size_t i = 0; i < v->val.l.count; ++i ) {
        LTWO_ARG_TYPES(v, "join", i, LVAL_QEXPR, LVAL_STR);
    }

    struct lval_t *a = lval_pop(v, 0);

    if ( a->type == LVAL_STR ) {
        while ( v->val.l.count ) {
            a = lval_join_str(a, lval_pop(v, 0));
        }
        lval_del(v);
        return a;
    } else {
        while ( v->val.l.count ) {
            a = lval_join(a, lval_pop(v, 0));
        }

        lval_del(v);
        return a;
    }
}

struct lval_t *builtin_cons(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "cons", 2);
    LARG_TYPE(v, "cons", 1, LVAL_QEXPR);

    struct lval_t *consvalue = lval_pop(v, 0);
    struct lval_t *collection = lval_pop(v, 0);

    lval_offer(collection, consvalue);

    lval_del(v);
    return collection;
}

struct lval_t *builtin_len(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "len", 1);
    LTWO_ARG_TYPES(v, "len", 0, LVAL_QEXPR, LVAL_STR);

    struct lval_t *arg = LGETCELL(v, 0);
    size_t count = 0;
    if ( arg->type == LVAL_STR ) {
        count = strlen(arg->val.strval);
    } else {
        count = arg->val.l.count;
    }

    lval_del(v);
    return lval_int(count);
}

struct lval_t *builtin_init(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "init", 1);
    LTWO_ARG_TYPES(v, "init", 0, LVAL_QEXPR, LVAL_STR);

    struct lval_t *collection = lval_pop(v, 0);

    if ( collection->type == LVAL_QEXPR ) {
        if ( collection->val.l.count > 0 ) {
            lval_del(lval_pop(collection, (collection->val.l.count - 1) ));
        }
    } else {
        size_t origsize = strlen(collection->val.strval);
        collection->val.strval[origsize - 1] = '\0';
        char *resized = realloc(collection->val.strval, origsize);
        if ( resized == NULL ) {
            perror("Could not resize char array");
            lval_del(v);
            lenv_del(e);
            exit(1);
        }
        collection->val.strval = resized;
    }

    lval_del(v);
    return collection;
}

/* * control flow builtins  * */

struct lval_t *builtin_exit(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "exit", 1);
    LARG_TYPE(v, "exit", 0, LVAL_INT);

    int exit_code = (int) (v->val.l.cells[0]->val.intval);
    lval_del(v);
    exit(exit_code);

    return lval_sexpr();
}

struct lval_t *builtin_if(struct lenv_t *e, struct lval_t *v) {
    LNUM_ARGS(v, "if", 3);
    LARG_TYPE(v, "if", 0, LVAL_BOOL);
    LARG_TYPE(v, "if", 1, LVAL_QEXPR);
    LARG_TYPE(v, "if", 2, LVAL_QEXPR);

    struct lval_t *res = NULL;
    struct lval_t *cond = v->val.l.cells[0];

    if ( cond->val.intval ) {
        res = lval_pop(v, 1);
        res->type = LVAL_SEXPR;
        res = lval_eval(e, res);
    } else {
        res = lval_pop(v, 2);
        res->type = LVAL_SEXPR;
        res = lval_eval(e, res);
    }

    lval_del(v);
    return res;
}

/* * reflection builtins * */

struct lval_t *builtin_type(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "type", 1);

    char *t = ltype_name(v->val.l.cells[0]->type);

    lval_del(v);
    return lval_str(t);
}

/* * function builtins * */

struct lval_t *builtin_lambda(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "\\", 2);
    LARG_TYPE(v, "\\", 0, LVAL_QEXPR);
    LARG_TYPE(v, "\\", 1, LVAL_QEXPR);

    struct lval_t *formals = v->val.l.cells[0];
    struct lval_t *body = v->val.l.cells[1];

    for ( size_t i = 0; i < formals->val.l.count; ++i ) {
       LASSERT(v, formals->val.l.cells[i]->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", 
            i + 1, ltype_name(LVAL_SYM), ltype_name(formals->val.l.cells[i]->type));
    }

    formals = lval_pop(v, 0);
    body = lval_pop(v, 0);
    lval_del(v);

    return lval_lambda(formals, body, lambda_env_prealloc);
}

/**
 *  Function declaration builtin
 */
struct lval_t *builtin_fn(struct lenv_t *e, struct lval_t*v) {
    LNUM_ARGS(v, "fn", 3);
    LARG_TYPE(v, "fn", 0, LVAL_QEXPR); // function name
    LARG_TYPE(v, "fn", 1, LVAL_QEXPR); // parameter list
    LARG_TYPE(v, "fn", 2, LVAL_QEXPR); // body 

    struct lval_t *name = LGETCELL(v, 0);
    struct lval_t *formals = LGETCELL(v, 1);
    struct lval_t *body = LGETCELL(v, 2);

    /* checking function name */
    LASSERT(v, name->val.l.count == 1, 
        "Function was parsed a empty function name. A function must be named", 0);
    LASSERT(v, LGETCELL(name, 0)->type == LVAL_SYM, 
        "Expected function name to be of type '%s'; got type '%s'.", 
        ltype_name(LVAL_SYM), ltype_name(LGETCELL(name, 0)->type));

    /* checking formals parameters */
    for ( size_t i = 0; i < formals->val.l.count; ++i ) {
       LASSERT(v, LGETCELL(formals, i)->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", 
            i + 1, ltype_name(LVAL_SYM), ltype_name(LGETCELL(formals, i)->type));
    }

    /* params are ok poping them off the value */
    name = lval_pop(v, 0);
    formals = lval_pop(v, 0);
    body = lval_pop(v, 0);

    struct lval_t *fn = lval_lambda(formals, body, fun_env_prealloc);

    lenv_put(e, LGETCELL(name, 0), fn);
    lval_del(fn);
    lval_del(name);
    lval_del(v);

    return lval_sexpr();
}

/* * value definition builtins * */

struct lval_t *builtin_var(struct lenv_t *, struct lval_t *, char *);

struct lval_t *builtin_def(struct lenv_t *e, struct lval_t *v) {
    return builtin_var(e, v, "def");
}

struct lval_t *builtin_put(struct lenv_t *e, struct lval_t *v) {
    return builtin_var(e, v, "=");
}

struct lval_t *builtin_var(struct lenv_t *e, struct lval_t *v, char *sym) {
    LARG_TYPE(v, sym, 0, LVAL_QEXPR);

    struct lval_t *names = LGETCELL(v, 0);

    for ( size_t i = 0; i < names->val.l.count; ++i ) {
        LASSERT(v, LGETCELL(names, i)->type == LVAL_SYM, "Function '%s' cannot assign value(s) to name(s). Name %lu is of type '%s'; expected type '%s'.", sym, i, ltype_name(LGETCELL(names, i)->type), ltype_name(LVAL_SYM));
    }

    LASSERT(v, names->val.l.count == v->val.l.count - 1, "Function '%s' cannot assign value(s) to name(s). Number of name(s) and value(s) does not match. Saw %lu name(s) expected %lu value(s).", sym, names->val.l.count, v->val.l.count - 1);

    for ( size_t i = 0; i < names->val.l.count; ++i ) {
        if ( strcmp(sym, "def") == 0 ) {
            lenv_def(e, LGETCELL(names, i), LGETCELL(v, i + 1));
        } else if ( strcmp(sym, "=") == 0 ) {
            lenv_put(e, LGETCELL(names, i), LGETCELL(v, i + 1));
        }
    }

    lval_del(v);
    return lval_sexpr();
}

/* * comparison builtins * */

struct lval_t *builtin_ord(struct lenv_t *e, struct lval_t *v, char *sym);

struct lval_t *builtin_lt(struct lenv_t *e, struct lval_t *v) {
    return builtin_ord(e, v, "<");
}

struct lval_t *builtin_gt(struct lenv_t *e, struct lval_t *v) {
    return builtin_ord(e, v, ">");
}

struct lval_t *builtin_le(struct lenv_t *e, struct lval_t *v) {
    return builtin_ord(e, v, "<=");
}

struct lval_t *builtin_ge(struct lenv_t *e, struct lval_t *v) {
    return builtin_ord(e, v, ">=");
}

struct lval_t *builtin_ord(struct lenv_t *e, struct lval_t *v, char *sym) {
    UNUSED(e);
    LNUM_ARGS(v, sym, 2);

    struct lval_t *lhs = v->val.l.cells[0];
    struct lval_t *rhs = v->val.l.cells[1];

    LASSERT(v, LIS_NUM(lhs->type), "Wrong type of argument parsed to '%s'. Expected '%s' or '%s' got '%s'.", sym, ltype_name(LVAL_INT), ltype_name(LVAL_FLOAT), ltype_name(lhs->type));
    LASSERT(v, LIS_NUM(rhs->type), "Wrong type of argument parsed to '%s'. Expected '%s' or '%s' got '%s'.", sym, ltype_name(LVAL_INT), ltype_name(LVAL_FLOAT), ltype_name(rhs->type));
    LASSERT(v, lhs->type == rhs->type, "Type of arguments does not match. Argument %i is of type '%s'; argument %i is of type '%s'.", 1, ltype_name(lhs->type), 2, ltype_name(rhs->type));

    long long res = 0;
    if ( strcmp(sym, "<") == 0 ) {
        res = (lhs->val.intval < rhs->val.intval);
    } else if ( strcmp(sym, ">") == 0 ) {
        res = (lhs->val.intval > rhs->val.intval);
    } else if ( strcmp(sym, "<=") == 0 ) {
        res = (lhs->val.intval <= rhs->val.intval);
    } else if ( strcmp(sym, ">=") == 0 ) {
        res = (lhs->val.intval >= rhs->val.intval);
    }

    lval_del(v);
    return lval_bool(res);
}

struct lval_t *builtin_cmp(struct lenv_t *e, struct lval_t *v, char *sym) {
    UNUSED(e);
    LNUM_ARGS(v, sym, 2);

    struct lval_t *lhs = v->val.l.cells[0];
    struct lval_t *rhs = v->val.l.cells[1];

    long long res = 0;
    if ( strcmp(sym, "==") == 0 ) {
        res = lval_eq(lhs, rhs);
    } else if ( strcmp(sym, "!=") == 0 ) {
        res = !lval_eq(lhs, rhs);
    }

    lval_del(v);
    return lval_bool(res);
}

struct lval_t *builtin_eq(struct lenv_t *e, struct lval_t *v) {
    return builtin_cmp(e, v, "==");
}

struct lval_t *builtin_ne(struct lenv_t *e, struct lval_t *v) {
    return builtin_cmp(e, v, "!=");
}

/* logical builtins */

struct lval_t *builtin_and(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "&&", 2);
    LARG_TYPE(v, "&&", 0, LVAL_BOOL);
    LARG_TYPE(v, "&&", 1, LVAL_BOOL);

    long long res = (v->val.l.cells[0]->val.intval && v->val.l.cells[1]->val.intval);

    lval_del(v);
    return lval_bool(res);
}

struct lval_t *builtin_or(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "||", 2);
    LARG_TYPE(v, "||", 0, LVAL_BOOL);
    LARG_TYPE(v, "||", 1, LVAL_BOOL);

    long long res = (v->val.l.cells[0]->val.intval || v->val.l.cells[1]->val.intval);

    lval_del(v);
    return lval_bool(res);
}

struct lval_t *builtin_not(struct lenv_t *e, struct lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "!", 1);
    LARG_TYPE(v, "!", 0, LVAL_BOOL);

    long long res = !v->val.l.cells[0]->val.intval;

    lval_del(v);
    return lval_bool(res);
}

/* source importation builtins */

struct lval_t *builtin_load(struct lenv_t *e, struct lval_t *v) {
    LNUM_ARGS(v, "load", 1);
    LARG_TYPE(v, "load", 0, LVAL_STR);

    mpc_result_t r;
    if ( mpc_parse_contents(LGETCELL(v, 0)->val.strval, elems.Lisper, &r) ) {

        struct lval_t *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while ( expr->val.l.count ) {
            struct lval_t *x = lval_eval(e, lval_pop(expr, 0));

            if ( x->type == LVAL_ERR ) {
                lval_println(x);
            }
            lval_del(x);
        }

        lval_del(expr);
        lval_del(v);

        return lval_sexpr();
    } else {
        /* parse error */
        char *err_msg = mpc_err_string(r.error);
        mpc_err_delete(r.error);

        struct lval_t *err = lval_err("Could not load library %s", err_msg);
        free(err_msg);
        lval_del(v);

        return err;
    }
}

void register_builtins(struct lenv_t *e) {
    LENV_BUILTIN(list);
    LENV_BUILTIN(head);
    LENV_BUILTIN(tail);
    LENV_BUILTIN(eval);
    LENV_BUILTIN(join);
    LENV_BUILTIN(cons);
    LENV_BUILTIN(len);
    LENV_BUILTIN(init);
    LENV_BUILTIN(def);
    LENV_BUILTIN(exit);
    LENV_BUILTIN(max);
    LENV_BUILTIN(min);
    LENV_BUILTIN(fn);
    LENV_BUILTIN(if);
    LENV_BUILTIN(args);
    LENV_BUILTIN(type);
    LENV_BUILTIN(load);
    LENV_BUILTIN(error);
    LENV_BUILTIN(print);
    LENV_BUILTIN(read);
    LENV_BUILTIN(show);
    LENV_BUILTIN(open);
    LENV_BUILTIN(close);
    LENV_BUILTIN(flush);
    LENV_BUILTIN(putstr);
    LENV_BUILTIN(rewind);
    LENV_BUILTIN(getstr);

    LENV_SYMBUILTIN("+", add);
    LENV_SYMBUILTIN("-", sub);
    LENV_SYMBUILTIN("*", mul);
    LENV_SYMBUILTIN("/", div);
    LENV_SYMBUILTIN("%", mod);
    LENV_SYMBUILTIN("\\", lambda);
    LENV_SYMBUILTIN("=", put);

    LENV_SYMBUILTIN("==", eq);
    LENV_SYMBUILTIN("!=", ne);

    LENV_SYMBUILTIN("||", or);
    LENV_SYMBUILTIN("&&", and);
    LENV_SYMBUILTIN("!", not);

    LENV_SYMBUILTIN(">", gt);
    LENV_SYMBUILTIN("<", lt);
    LENV_SYMBUILTIN(">=", ge);
    LENV_SYMBUILTIN("<=", le);
}

