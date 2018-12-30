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

#define LASSERT(args, cond, fmt, ...) \
    if ( !(cond) ) { lval_t *err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err;  }

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

extern grammar_elems elems;
extern struct argument_capture *args;

/* env preallocated sizes */
const size_t lambda_env_prealloc = 50;
const size_t fun_env_prealloc = 200;

/* * math builtins * */

lval_t *builtin_op(lenv_t *e, lval_t *v, char *sym) {
    UNUSED(e);

    ltype last_type = v->val.l.cells[0]->type;

    if ( !LIS_NUM(v->val.l.cells[0]->type) ) {
        lval_t *err = lval_err("Cannot operate on argument %i. Non-number type '%s' parsed to %s.", 1, ltype_name(v->val.l.cells[0]->type), sym);
        lval_del(v);
        return err;
    }

    for ( size_t i = 1; i < v->val.l.count; i++ ) {
        if ( !LIS_NUM(v->val.l.cells[i]->type) ) {
            lval_t *err = lval_err("Cannot operate on argument %lu. Non-number type '%s' parsed to %s.", i + 1, ltype_name(v->val.l.cells[i]->type), sym);
            lval_del(v);
            return err;
        }
        if ( last_type != v->val.l.cells[i]->type ) {
            lval_t *err = lval_err("Argument type mismatch. Argument %lu is of type '%s', while argument %lu is of type '%s'.", i, ltype_name(last_type), i + 1, ltype_name(v->val.l.cells[i]->type));
            lval_del(v);
            return err;
        }
        last_type = v->val.l.cells[i]->type;
    }

    lval_t *a = lval_pop(v, 0);

    if ( a->type == LVAL_INT ) {
        /* int val */
        if ( strcmp(sym, "-") == 0 && v->val.l.count == 0 ) {
            a->val.intval = (-a->val.intval);
        }

        while ( v->val.l.count > 0 ) {
            lval_t *b = lval_pop(v, 0);

            if ( strcmp(sym, "+") == 0 ) {
                a->val.intval += b->val.intval;
            } else if ( strcmp(sym, "-") == 0 ) {
                a->val.intval -= b->val.intval;
            } else if ( strcmp(sym, "*") == 0 ) {
                a->val.intval *= b->val.intval;
            } else if ( strcmp(sym, "/") == 0 ) {
                if ( b->val.intval == 0 ) {
                    lval_del(a);
                    lval_del(b);
                    a = lval_err("Division by zero");
                    break;
                }
                a->val.intval /= b->val.intval;
            } else if ( strcmp(sym, "%") == 0 ) {
                if ( b->val.intval == 0 ) {
                    lval_del(a);
                    lval_del(b);
                    a = lval_err("Division by zero");
                    break;
                }
                a->val.intval %= b->val.intval;
            } else if ( strcmp(sym, "min") == 0 ) {
                if ( a->val.intval > b->val.intval ) {
                    a->val.intval = b->val.intval;
                }
            } else if ( strcmp(sym, "max") == 0 )  {
                if ( a->val.intval < b->val.intval ) {
                    a->val.intval = b->val.intval;
                }
            }
            lval_del(b);
        }

    } else {
        /* Floating point */
        if ( strcmp(sym, "-") == 0 && v->val.l.count == 0 ) {
            a->val.floatval = (-a->val.floatval);
        }

        while ( v->val.l.count > 0 ) {
            lval_t *b = lval_pop(v, 0);

            if ( strcmp(sym, "+") == 0 ) {
                a->val.floatval += b->val.floatval;
            } else if ( strcmp(sym, "-") == 0 ) {
                a->val.floatval -= b->val.floatval;
            } else if ( strcmp(sym, "*") == 0 ) {
                a->val.floatval *= b->val.floatval;
            } else if ( strcmp(sym, "/") == 0 ) {
                if ( b->val.floatval == 0 ) {
                    lval_del(a);
                    lval_del(b);
                    a = lval_err("Division by zero");
                    break;
                }
                a->val.floatval /= b->val.floatval;
            } else if ( strcmp(sym, "%") == 0 ) {
                if ( b->val.floatval == 0 ) {
                    lval_del(a);
                    lval_del(b);
                    a = lval_err("Division by zero");
                    break;
                }
                a->val.floatval = fmod(a->val.floatval, b->val.floatval);
            } else if ( strcmp(sym, "min") == 0 ) {
                if ( a->val.floatval > b->val.floatval ) {
                    a->val.floatval = b->val.floatval;
                }
            } else if ( strcmp(sym, "max") == 0 )  {
                if ( a->val.floatval < b->val.floatval ) {
                    a->val.floatval = b->val.floatval;
                }
            }
            lval_del(b);
        }
    }

    lval_del(v);
    return a;
}

/* math operators */

lval_t *builtin_add(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "+");
}

lval_t *builtin_sub(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "-");
}

lval_t *builtin_mul(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "*");
}

lval_t *builtin_div(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "/");
}

lval_t *builtin_pow(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "^");
}

lval_t *builtin_mod(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "%");
}

lval_t *builtin_min(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "min");
}

lval_t *builtin_max(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "max");
}

/* * q-expression specific builtins * */

/**
 * Convert expression into a q-expression
 */
lval_t *builtin_list(lenv_t *e, lval_t *v) {
    UNUSED(e);
    v->type = LVAL_QEXPR;
    return v;
}

/**
 * Take one q-expression and evaluate it
 * as a s-expression.
 */
lval_t *builtin_eval(lenv_t *e, lval_t *v) {
    LNUM_ARGS(v, "eval", 1);
    LARG_TYPE(v, "eval", 0, LVAL_QEXPR);

    lval_t *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(e, a);
}

/**
 * Read a string and try and parse into a
 * lval
 */
lval_t *builtin_read(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "read", 1);
    LARG_TYPE(v, "read", 0, LVAL_STR);

    mpc_result_t r;
    if ( mpc_parse("input", LGETCELL(v, 0)->val.strval, elems.Lisper, &r) ) {
        lval_t *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        lval_del(v);
        return builtin_list(e, expr);
    }

    /* parse error */
    char *err_msg = mpc_err_string(r.error);
    mpc_err_delete(r.error);

    lval_t *err = lval_err("Could parse str %s", err_msg);
    free(err_msg);
    lval_del(v);

    return err;
}

/**
 * Print the contents of a string
 */
lval_t *builtin_show(lenv_t *e, lval_t *v) {
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
lval_t *builtin_args(lenv_t *e, lval_t *v) {
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
lval_t *builtin_print(lenv_t *e, lval_t *v) {
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
lval_t *builtin_open(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "open", 2);
    LARG_TYPE(v, "open", 0, LVAL_STR);
    LARG_TYPE(v, "open", 1, LVAL_STR);

    lval_t *filename = lval_pop(v, 0);
    lval_t *mode = lval_pop(v, 0);

    char *m = mode->val.strval;
    char *path = filename->val.strval;
    FILE *fp;

    if ( !(strcmp(m, "r") == 0 ||
           strcmp(m, "r+") == 0 ||
           strcmp(m, "w") == 0 ||
           strcmp(m, "w+") == 0 ||
           strcmp(m, "a") == 0 ||
           strcmp(m, "a+") == 0) ) {
        lval_t *err = lval_err("Mode not set to either 'r', 'r+', 'w', 'w+', 'a' or 'a+'");
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
lval_t *builtin_close(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "close", 1);
    LARG_TYPE(v, "close", 0, LVAL_FILE);

    lval_t *f = LGETCELL(v, 0);

    if ( fclose(f->val.file->fp) != 0 ) {
        lval_del(v);
        return lval_err("Cloud not close file: '%s'", f->val.file->path);
    }

    lval_del(v);
    return lval_sexpr();
}

lval_t *builtin_flush(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "flush", 1);
    LARG_TYPE(v, "flush", 0, LVAL_FILE);

    lval_t *f = LGETCELL(v, 0);

    if ( fflush(f->val.file->fp) != 0 ) {
        lval_del(v);
        return lval_err("Cloud not flush file buffer for: '%s'", f->val.file->path);
    }

    lval_del(v);
    return lval_sexpr();
}

lval_t *builtin_putstr(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "putstr", 2);
    LARG_TYPE(v, "putstr", 0, LVAL_STR);
    LARG_TYPE(v, "putstr", 1, LVAL_FILE);

    lval_t *f = LGETCELL(v, 1);
    lval_t *str = LGETCELL(v, 0);

    if ( fputs(str->val.strval, f->val.file->fp) == EOF ) {
        lval_del(v);
        return lval_err("Could write '%s' to file", str->val.strval);
    }

    return lval_sexpr();
}

lval_t *builtin_getstr(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "getstr", 1);
    LARG_TYPE(v, "getstr", 0, LVAL_FILE);

    lval_t *f = LGETCELL(v, 0);

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

lval_t *builtin_rewind(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "rewind", 1);
    LARG_TYPE(v, "rewind", 0, LVAL_FILE);

    rewind(LGETCELL(v, 0)->val.file->fp);

    return lval_sexpr();
}

/* * error builtins * */

lval_t *builtin_error(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "error", 1);
    LARG_TYPE(v, "error", 0, LVAL_STR);

    lval_t *err = lval_err(LGETCELL(v, 0)->val.strval);

    lval_del(v);
    return err;
}


/* * collection builtins * */

lval_t *builtin_tail(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "tail", 1);
    LTWO_ARG_TYPES(v, "tail", 0, LVAL_QEXPR, LVAL_STR);

    lval_t *a = lval_take(v, 0);

    if ( a->type == LVAL_STR ) {
        if ( strlen(a->val.strval) > 1 ) {
            lval_t *tail = lval_str(a->val.strval + 1);
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

lval_t *builtin_head(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "head", 1);
    LTWO_ARG_TYPES(v, "head", 0, LVAL_QEXPR, LVAL_STR);

    lval_t *a = lval_take(v, 0);

    if ( a->type == LVAL_STR ) {
        if ( strlen(a->val.strval) > 1 ) {
            char second = a->val.strval[1];
            a->val.strval[1] = '\0';

            lval_t *head = lval_str(a->val.strval);
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

lval_t *builtin_join(lenv_t *e, lval_t *v) {
    UNUSED(e);
    for ( size_t i = 0; i < v->val.l.count; ++i ) {
        LTWO_ARG_TYPES(v, "join", i, LVAL_QEXPR, LVAL_STR);
    }

    lval_t *a = lval_pop(v, 0);

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

lval_t *builtin_cons(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "cons", 2);
    LARG_TYPE(v, "cons", 1, LVAL_QEXPR);

    lval_t *consvalue = lval_pop(v, 0);
    lval_t *collection = lval_pop(v, 0);

    lval_offer(collection, consvalue);

    lval_del(v);
    return collection;
}

lval_t *builtin_len(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "len", 1);
    LTWO_ARG_TYPES(v, "len", 0, LVAL_QEXPR, LVAL_STR);

    lval_t *arg = LGETCELL(v, 0);
    size_t count = 0;
    if ( arg->type == LVAL_STR ) {
        count = strlen(arg->val.strval);
    } else {
        count = arg->val.l.count;
    }

    lval_del(v);
    return lval_int(count);
}

lval_t *builtin_init(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "init", 1);
    LTWO_ARG_TYPES(v, "init", 0, LVAL_QEXPR, LVAL_STR);

    lval_t *collection = lval_pop(v, 0);

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

lval_t *builtin_exit(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "exit", 1);
    LARG_TYPE(v, "exit", 0, LVAL_INT);

    int exit_code = (int) (v->val.l.cells[0]->val.intval);
    lval_del(v);
    exit(exit_code);

    return lval_sexpr();
}

lval_t *builtin_if(lenv_t *e, lval_t *v) {
    LNUM_ARGS(v, "if", 3);
    LARG_TYPE(v, "if", 0, LVAL_BOOL);
    LARG_TYPE(v, "if", 1, LVAL_QEXPR);
    LARG_TYPE(v, "if", 2, LVAL_QEXPR);

    lval_t *res = NULL;
    lval_t *cond = v->val.l.cells[0];

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

lval_t *builtin_type(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "type", 1);

    char *t = ltype_name(v->val.l.cells[0]->type);

    lval_del(v);
    return lval_str(t);
}

/* * function builtins * */

lval_t *builtin_lambda(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "\\", 2);
    LARG_TYPE(v, "\\", 0, LVAL_QEXPR);
    LARG_TYPE(v, "\\", 1, LVAL_QEXPR);

    lval_t *formals = v->val.l.cells[0];
    lval_t *body = v->val.l.cells[1];

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
lval_t *builtin_fn(lenv_t *e, lval_t*v) {
    LNUM_ARGS(v, "fn", 3);
    LARG_TYPE(v, "fn", 0, LVAL_QEXPR); // function name
    LARG_TYPE(v, "fn", 1, LVAL_QEXPR); // parameter list
    LARG_TYPE(v, "fn", 2, LVAL_QEXPR); // body 

    lval_t *name = LGETCELL(v, 0);
    lval_t *formals = LGETCELL(v, 1);
    lval_t *body = LGETCELL(v, 2);

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

    lval_t *fn = lval_lambda(formals, body, fun_env_prealloc);

    lenv_put(e, LGETCELL(name, 0), fn);
    lval_del(fn);
    lval_del(name);
    lval_del(v);

    return lval_sexpr();
}

/* * value definition builtins * */

lval_t *builtin_var(lenv_t *, lval_t *, char *);

lval_t *builtin_def(lenv_t *e, lval_t *v) {
    return builtin_var(e, v, "def");
}

lval_t *builtin_put(lenv_t *e, lval_t *v) {
    return builtin_var(e, v, "=");
}

lval_t *builtin_var(lenv_t *e, lval_t *v, char *sym) {
    LARG_TYPE(v, sym, 0, LVAL_QEXPR);

    lval_t *names = LGETCELL(v, 0);

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

lval_t *builtin_ord(lenv_t *e, lval_t *v, char *sym);

lval_t *builtin_lt(lenv_t *e, lval_t *v) {
    return builtin_ord(e, v, "<");
}

lval_t *builtin_gt(lenv_t *e, lval_t *v) {
    return builtin_ord(e, v, ">");
}

lval_t *builtin_le(lenv_t *e, lval_t *v) {
    return builtin_ord(e, v, "<=");
}

lval_t *builtin_ge(lenv_t *e, lval_t *v) {
    return builtin_ord(e, v, ">=");
}

lval_t *builtin_ord(lenv_t *e, lval_t *v, char *sym) {
    UNUSED(e);
    LNUM_ARGS(v, sym, 2);

    lval_t *lhs = v->val.l.cells[0];
    lval_t *rhs = v->val.l.cells[1];

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

lval_t *builtin_cmp(lenv_t *e, lval_t *v, char *sym) {
    UNUSED(e);
    LNUM_ARGS(v, sym, 2);

    lval_t *lhs = v->val.l.cells[0];
    lval_t *rhs = v->val.l.cells[1];

    long long res = 0;
    if ( strcmp(sym, "==") == 0 ) {
        res = lval_eq(lhs, rhs);
    } else if ( strcmp(sym, "!=") == 0 ) {
        res = !lval_eq(lhs, rhs);
    }

    lval_del(v);
    return lval_bool(res);
}

lval_t *builtin_eq(lenv_t *e, lval_t *v) {
    return builtin_cmp(e, v, "==");
}

lval_t *builtin_ne(lenv_t *e, lval_t *v) {
    return builtin_cmp(e, v, "!=");
}

/* logical builtins */

lval_t *builtin_and(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "&&", 2);
    LARG_TYPE(v, "&&", 0, LVAL_BOOL);
    LARG_TYPE(v, "&&", 1, LVAL_BOOL);

    long long res = (v->val.l.cells[0]->val.intval && v->val.l.cells[1]->val.intval);

    lval_del(v);
    return lval_bool(res);
}

lval_t *builtin_or(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "||", 2);
    LARG_TYPE(v, "||", 0, LVAL_BOOL);
    LARG_TYPE(v, "||", 1, LVAL_BOOL);

    long long res = (v->val.l.cells[0]->val.intval || v->val.l.cells[1]->val.intval);

    lval_del(v);
    return lval_bool(res);
}

lval_t *builtin_not(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LNUM_ARGS(v, "!", 1);
    LARG_TYPE(v, "!", 0, LVAL_BOOL);

    long long res = !v->val.l.cells[0]->val.intval;

    lval_del(v);
    return lval_bool(res);
}

/* source importation builtins */

lval_t *builtin_load(lenv_t *e, lval_t *v) {
    LNUM_ARGS(v, "load", 1);
    LARG_TYPE(v, "load", 0, LVAL_STR);

    mpc_result_t r;
    if ( mpc_parse_contents(LGETCELL(v, 0)->val.strval, elems.Lisper, &r) ) {

        lval_t *expr = lval_read(r.output);
        mpc_ast_delete(r.output);

        while ( expr->val.l.count ) {
            lval_t *x = lval_eval(e, lval_pop(expr, 0));

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

        lval_t *err = lval_err("Could not load library %s", err_msg);
        free(err_msg);
        lval_del(v);

        return err;
    }
}

void register_builtins(lenv_t *e) {
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
    LENV_SYMBUILTIN("^", pow);
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

