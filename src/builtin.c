#include <stdlib.h>
#include <math.h>
#include "builtin.h"
#include "lval.h"
#include "lenv.h"
#include "prompt.h"

#define UNUSED(x) (void)(x)

#define LASSERT(args, cond, fmt, ...) \
    if ( !(cond) ) { lval_t *err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err;  }

#define LLEAST_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l->count >= numargs, "Not enough arguments parsed to '%s'. Expected at least %lu argument(s); got %lu.", funcname, numargs, sym->val.l->count)

#define LMOST_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l->count <= numargs, "Too many arguments parsed to '%s'. Expected at most %lu argument(s); got %lu.", funcname,  numargs, sym->val.l->count)

#define LEXACT_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l->count == numargs, "Wrong number of arguments parsed to '%s'. Expected at exactly %lu argument(s); got %lu. ", funcname, numargs, sym->val.l->count)

#define LNOT_EMPTY_QEXPR(sym, funcname, i) \
    LASSERT(sym, sym->val.l->cells[i]->type == LVAL_QEXPR && sym->val.l->cells[i]->val.l->count > 0, "Empty %s parsed to '%s'.", ltype_name(sym->val.l->cells[i]->type, funcname))

#define LWRONG_ARG_TYPE(sym, funcname, i, expected) \
    LASSERT(sym, sym->val.l->cells[i]->type == expected, "Wrong type of argument parsed to '%s'. Expected '%s' got '%s'.", funcname, ltype_name(expected), ltype_name(sym->val.l->cells[i]->type));


lval_t *builtin_op(lenv_t *e, lval_t *v, char *sym) {
    UNUSED(e);
    lcells_t *c = v->val.l;

    for ( size_t i = 0; i < c->count; i++ ) {
        if ( c->cells[i]->type != LVAL_NUM ) {
            lval_del(v);
            return lval_err("Cannot operate on argument %lu. Non-number type '%s' parsed to %s.", i + 1, ltype_name(c->cells[i]->type), sym);
        }
    }

    lval_t *a = lval_pop(v, 0);

    if ( strcmp(sym, "-") == 0 && c->count == 0 ) {
        a->val.num = -a->val.num;
    }

    while ( v->val.l->count > 0 ) {

        lval_t *b = lval_pop(v, 0);

        if ( strcmp(sym, "+") == 0 ) {
            a->val.num += b->val.num;
        } else if ( strcmp(sym, "-") == 0 ) {
            a->val.num -= b->val.num;
        } else if ( strcmp(sym, "*") == 0 ) {
            a->val.num *= b->val.num;
        } else if ( strcmp(sym, "/") == 0 ) {
            if ( b->val.num == 0 ) {
                lval_del(a);
                lval_del(b);
                a = lval_err("Division by zero");
                break;
            }
            a->val.num /= b->val.num;
        } else if ( strcmp(sym, "%") == 0 ) {
            if ( b->val.num == 0 ) {
                lval_del(a);
                lval_del(b);
                a = lval_err("Division by zero");
                break;
            }
            a->val.num = fmod(a->val.num, b->val.num);
        } else if ( strcmp(sym, "min") == 0 ) {
            if ( a->val.num > b->val.num ) {
                a->val.num = b->val.num;
            }
        } else if ( strcmp(sym, "max") == 0 )  {
            if ( a->val.num < b->val.num ) {
                a->val.num = b->val.num;
            }
        }
        lval_del(b);
    }

    lval_del(v);
    return a;
}

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

lval_t *builtin_fmod(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "%");
}

lval_t *builtin_min(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "min");
}

lval_t *builtin_max(lenv_t *e, lval_t *a) {
    return builtin_op(e, a, "max");
}

lval_t *builtin_tail(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "tail", 1);
    LWRONG_ARG_TYPE(v, "tail", 0, LVAL_QEXPR);

    lval_t *a = lval_take(v, 0);
    if ( a->val.l->count > 0 ) {
        lval_del(lval_pop(a, 0));
    }
    return a;
}

lval_t *builtin_head(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "head", 1);
    LWRONG_ARG_TYPE(v, "head", 0, LVAL_QEXPR);

    lval_t *a = lval_take(v, 0);

    while ( a->val.l->count > 1 ) {
        lval_del(lval_pop(a, 1));
    }

    return a;
}

lval_t *builtin_list(lenv_t *e, lval_t *v) {
    UNUSED(e);
    v->type = LVAL_QEXPR;
    return v;
}

lval_t *builtin_eval(lenv_t *e, lval_t *v) {
    LEXACT_ARGS(v, "eval", 1);
    LWRONG_ARG_TYPE(v, "eval", 0, LVAL_QEXPR);

    lval_t *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(e, a);
}


lval_t *builtin_join(lenv_t *e, lval_t *v) {
    UNUSED(e);
    for ( size_t i = 0; i < v->val.l->count; ++i ) {
        LWRONG_ARG_TYPE(v, "join", i, LVAL_QEXPR);
    }

    lval_t *a = lval_pop(v, 0);
    while ( v->val.l->count ) {
        a = lval_join(a, lval_pop(v, 0));
    }

    lval_del(v);

    return a;
}

lval_t *builtin_cons(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "cons", 2);
    LWRONG_ARG_TYPE(v, "cons", 1, LVAL_QEXPR);

    lval_t *otherval = lval_pop(v, 0);
    lval_t *qexpr = lval_pop(v, 0);

    lval_offer(qexpr, otherval);

    lval_del(v);
    return qexpr;
}

lval_t *builtin_len(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "len", 1);
    LWRONG_ARG_TYPE(v, "len", 0, LVAL_QEXPR);

    lval_t *arg = lval_pop(v, 0);
    size_t count = arg->val.l->count;

    while ( arg->val.l->count ) {
        lval_del(lval_pop(arg, 0));
    }

    arg->type = LVAL_NUM;
    arg->val.num = ( (double) count ); // we only have double defined as numbers
    lval_del(v);

    return arg;
}

lval_t *builtin_init(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "init", 1);
    LWRONG_ARG_TYPE(v, "init", 0, LVAL_QEXPR);

    lval_t *qexpr = lval_pop(v, 0);
    if ( qexpr->val.l->count > 0 ) {
        lval_del(lval_pop(qexpr, (qexpr->val.l->count - 1) ));
    }
    lval_del(v);
    return qexpr;
}

lval_t *builtin_exit(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "exit", 1);
    LWRONG_ARG_TYPE(v, "exit", 0, LVAL_NUM);

    int exit_code = floor(v->val.l->cells[0]->val.num);
    lval_del(v);
    lenv_del(e);
    exit(exit_code);

    return NULL;
}

lval_t *builtin_lambda(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "\\", 2);
    LWRONG_ARG_TYPE(v, "\\", 0, LVAL_QEXPR);
    LWRONG_ARG_TYPE(v, "\\", 1, LVAL_QEXPR);

    lcells_t *args = v->val.l;
    lval_t *formals = args->cells[0];
    lval_t *body = args->cells[1];

    for ( size_t i = 0; i < formals->val.l->count; ++i ) {
       LASSERT(v, formals->val.l->cells[i]->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", i + 1, ltype_name(LVAL_SYM), ltype_name(formals->val.l->cells[i]->type));
    }

    formals = lval_pop(v, 0);
    body = lval_pop(v, 0);
    lval_del(v);

    return lval_lambda(formals, body);
}

lval_t *builtin_var(lenv_t *, lval_t *, char *);

lval_t *builtin_def(lenv_t *e, lval_t *v) {
    UNUSED(e);
    return builtin_var(e, v, "def");
}

lval_t *builtin_put(lenv_t *e, lval_t *v) {
    UNUSED(e);
    return builtin_var(e, v, "=");
}

lval_t *builtin_var(lenv_t *e, lval_t *v, char *sym) {
    LEXACT_ARGS(v, sym, 2);
    LWRONG_ARG_TYPE(v, sym, 0, LVAL_QEXPR);

    lval_t *names = v->val.l->cells[0];

    for ( size_t i = 0; i < names->val.l->count; ++i ) {
        LASSERT(v, names->val.l->cells[i]->type == LVAL_SYM, "Function '%s' cannot assign value(s) to name(s). Name %lu is of type '%s'; expected type '%s'.", sym, i, ltype_name(names->val.l->cells[i]->type), ltype_name(LVAL_SYM));
    }

    LASSERT(v, names->val.l->count == v->val.l->count - 1, "Function '%s' cannot assign value(s) to name(s). Number of name(s) and value(s) does not match. Saw %lu name(s) expected %lu value(s).", sym, names->val.l->count, v->val.l->count - 1);

    for ( size_t i = 0; i < names->val.l->count; ++i ) {
        if ( strcmp(sym, "def") == 0 ) {
            lenv_def(e, names->val.l->cells[i], v->val.l->cells[i + 1]);
        } else if ( strcmp(sym, "=") == 0 ) {
            lenv_put(e, names->val.l->cells[i], v->val.l->cells[i + 1]);
        }
    }

    lval_del(v);
    return lval_sexpr();
}

lval_t *lval_call(lenv_t *e, lval_t *f, lval_t *v) {

    if ( f->type == LVAL_BUILTIN ) {
        return f->val.builtin(e, v);
    }

    lfunc_t *func = f->val.fun;
    lcells_t *args = v->val.l;
    lcells_t *formals = func->formals->val.l;

    size_t given = args->count;
    size_t total = formals->count;

    while ( args->count > 0 ) {

        if ( formals->count == 0 &&
            args->cells[0]->type != LVAL_SEXPR ) {
            lval_del(v);
            return lval_err(
                "Function passed too many arguments; "
                "got %lu expected %lu",
                given,
                total
            );
        } else if ( formals->count == 0 &&
            args->cells[0]->type == LVAL_SEXPR &&
            args->cells[0]->val.l->count == 0 ) {
            /* empty sexpr to zero parameter function just calls it */
            break;
        }

        lval_t *sym = lval_pop(func->formals, 0);
        lval_t *val = lval_pop(v, 0);

        lenv_put(func->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    lval_del(v);

    if ( formals->count == 0 ) {
        func->env->parent = e;
        return builtin_eval( func->env, lval_add(lval_sexpr(), lval_copy(func->body)) );
    }
    return lval_copy(f);
}

lval_t *lval_eval_sexpr(lenv_t *e, lval_t *v) {
    lcells_t *symc;

    /* depth-first eval of sexpr */
    symc = v->val.l;
    for ( size_t i = 0; i < symc->count; i++ ) {
        symc->cells[i] = lval_eval(e, symc->cells[i]);
    }

    /* return first error */
    for ( size_t i = 0; i < symc->count; i++ ) {
        if ( symc->cells[i]->type == LVAL_ERR ) {
            return lval_take(v, i);
        }
    }

    /* empty sexpr */
    if ( symc->count == 0 ) {
        return v;
    }

    /* hoist first lval if only one is available */
    if ( symc->count == 1 ) {
        return lval_take(v, 0);
    }

    lval_t *f = lval_pop(v, 0);
    lval_t *res = NULL;
    char *t = NULL;

    /* function evaluation */
    switch ( f->type ) {
        case LVAL_LAMBDA:
        case LVAL_BUILTIN:
            res = lval_call(e, f, v);
            break;
        default:
            t = ltype_name(f->type);
            lval_del(f);
            lval_del(v);
            return lval_err("Expected first argument of %s to be of type '%s'; got '%s'.", ltype_name(LVAL_SEXPR), ltype_name(LVAL_BUILTIN), t);
    }

    lval_del(f);
    return res;
}

lval_t *lval_eval(lenv_t *e, lval_t *v) {
    lval_t *x;
    switch ( v->type ) {
        case LVAL_SYM:
            x = lenv_get(e, v);
            lval_del(v);
            return x;

        case LVAL_SEXPR:
            return lval_eval_sexpr(e, v);
        default:
            break;
    }

    return v;
}


