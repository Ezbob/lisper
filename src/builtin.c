#include <stdlib.h>
#include <math.h>
#include "builtin.h"
#include "lval.h"
#include "lenv.h"
#include "prompt.h"

#define UNUSED(x) (void)(x)

#define LIS_NUM(type) (type == LVAL_INT || type == LVAL_FLOAT)

#define LASSERT(args, cond, fmt, ...) \
    if ( !(cond) ) { lval_t *err = lval_err(fmt, ##__VA_ARGS__); lval_del(args); return err;  }

#define LLEAST_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l.count >= numargs, "Not enough arguments parsed to '%s'. Expected at least %lu argument(s); got %lu.", funcname, numargs, sym->val.l.count)

#define LMOST_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l.count <= numargs, "Too many arguments parsed to '%s'. Expected at most %lu argument(s); got %lu.", funcname,  numargs, sym->val.l.count)

#define LEXACT_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l.count == numargs, "Wrong number of arguments parsed to '%s'. Expected at exactly %lu argument(s); got %lu. ", funcname, numargs, sym->val.l.count)

#define LNOT_EMPTY_QEXPR(sym, funcname, i) \
    LASSERT(sym, sym->val.l.cells[i]->type == LVAL_QEXPR && sym->val.l.cells[i]->val.l.count > 0, "Empty %s parsed to '%s'.", ltype_name(sym->val.l.cells[i]->type, funcname))

#define LARG_TYPE(sym, funcname, i, expected) \
    LASSERT(sym, sym->val.l.cells[i]->type == expected, "Wrong type of argument parsed to '%s'. Expected '%s' got '%s'.", funcname, ltype_name(expected), ltype_name(sym->val.l.cells[i]->type));


lval_t *builtin_op(lenv_t *e, lval_t *v, char *sym) {
    UNUSED(e);

    for ( size_t i = 0; i < v->val.l.count; i++ ) {
        if ( !LIS_NUM(v->val.l.cells[i]->type) ) {
            lval_t *err = lval_err("Cannot operate on argument %lu. Non-number type '%s' parsed to %s.", i + 1, ltype_name(v->val.l.cells[i]->type), sym);
            lval_del(v);
            return err;
        }
    }

    lval_t *a = lval_pop(v, 0);

    if ( a->type == LVAL_INT ) {
        /* int val */
        if ( strcmp(sym, "-") == 0 && v->val.l.count == 0 ) {
            a->val.intval = -a->val.intval;
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
            a->val.floatval = -a->val.floatval;
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
    LARG_TYPE(v, "tail", 0, LVAL_QEXPR);

    lval_t *a = lval_take(v, 0);
    if ( a->val.l.count > 0 ) {
        lval_del(lval_pop(a, 0));
    }
    return a;
}

lval_t *builtin_head(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "head", 1);
    LARG_TYPE(v, "head", 0, LVAL_QEXPR);

    lval_t *a = lval_take(v, 0);

    while ( a->val.l.count > 1 ) {
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
    LARG_TYPE(v, "eval", 0, LVAL_QEXPR);

    lval_t *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(e, a);
}


lval_t *builtin_join(lenv_t *e, lval_t *v) {
    UNUSED(e);
    for ( size_t i = 0; i < v->val.l.count; ++i ) {
        LARG_TYPE(v, "join", i, LVAL_QEXPR);
    }

    lval_t *a = lval_pop(v, 0);
    while ( v->val.l.count ) {
        a = lval_join(a, lval_pop(v, 0));
    }

    lval_del(v);

    return a;
}

lval_t *builtin_cons(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "cons", 2);
    LARG_TYPE(v, "cons", 1, LVAL_QEXPR);

    lval_t *otherval = lval_pop(v, 0);
    lval_t *qexpr = lval_pop(v, 0);

    lval_offer(qexpr, otherval);

    lval_del(v);
    return qexpr;
}

lval_t *builtin_len(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "len", 1);
    LARG_TYPE(v, "len", 0, LVAL_QEXPR);

    lval_t *arg = lval_pop(v, 0);
    size_t count = arg->val.l.count;

    while ( arg->val.l.count ) {
        lval_del(lval_pop(arg, 0));
    }

    arg->type = LVAL_INT;
    arg->val.intval = ( (long long) count ); // we only have double defined as numbers
    lval_del(v);

    return arg;
}

lval_t *builtin_init(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "init", 1);
    LARG_TYPE(v, "init", 0, LVAL_QEXPR);

    lval_t *qexpr = lval_pop(v, 0);
    if ( qexpr->val.l.count > 0 ) {
        lval_del(lval_pop(qexpr, (qexpr->val.l.count - 1) ));
    }
    lval_del(v);
    return qexpr;
}

lval_t *builtin_exit(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "exit", 1);
    LARG_TYPE(v, "exit", 0, LVAL_INT);

    int exit_code = (int) (v->val.l.cells[0]->val.intval);
    /* TODO MORE CLEAN UP NEEDED */
    lval_del(v);
    lenv_del(e);
    exit(exit_code);

    return lval_sexpr();
}

lval_t *builtin_lambda(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "\\", 2);
    LARG_TYPE(v, "\\", 0, LVAL_QEXPR);
    LARG_TYPE(v, "\\", 1, LVAL_QEXPR);

    lval_t *formals = v->val.l.cells[0];
    lval_t *body = v->val.l.cells[1];

    for ( size_t i = 0; i < formals->val.l.count; ++i ) {
       LASSERT(v, formals->val.l.cells[i]->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", i + 1, ltype_name(LVAL_SYM), ltype_name(formals->val.l.cells[i]->type));
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
    LARG_TYPE(v, sym, 0, LVAL_QEXPR);

    lval_t *names = v->val.l.cells[0];

    for ( size_t i = 0; i < names->val.l.count; ++i ) {
        LASSERT(v, names->val.l.cells[i]->type == LVAL_SYM, "Function '%s' cannot assign value(s) to name(s). Name %lu is of type '%s'; expected type '%s'.", sym, i, ltype_name(names->val.l.cells[i]->type), ltype_name(LVAL_SYM));
    }

    LASSERT(v, names->val.l.count == v->val.l.count - 1, "Function '%s' cannot assign value(s) to name(s). Number of name(s) and value(s) does not match. Saw %lu name(s) expected %lu value(s).", sym, names->val.l.count, v->val.l.count - 1);

    for ( size_t i = 0; i < names->val.l.count; ++i ) {
        if ( strcmp(sym, "def") == 0 ) {
            lenv_def(e, names->val.l.cells[i], v->val.l.cells[i + 1]);
        } else if ( strcmp(sym, "=") == 0 ) {
            lenv_put(e, names->val.l.cells[i], v->val.l.cells[i + 1]);
        }
    }

    lval_del(v);
    return lval_sexpr();
}

lval_t *builtin_fun(lenv_t *e, lval_t*v) {
    LEXACT_ARGS(v, "fun", 2);
    LARG_TYPE(v, "fun", 0, LVAL_QEXPR);
    LARG_TYPE(v, "fun", 1, LVAL_QEXPR);

    lval_t *formals = v->val.l.cells[0];

    LASSERT(v, formals->val.l.count > 0, "Expected argument list be of least size %i.", 1);
    LASSERT(v, formals->val.l.cells[0]->type == LVAL_SYM, "Expected function name to be of type '%s'; got '%s'", ltype_name(LVAL_SYM), ltype_name(formals->val.l.cells[0]->type));

    lval_t *body = v->val.l.cells[1];

    for ( size_t i = 1; i < formals->val.l.count; ++i ) {
       LASSERT(v, formals->val.l.cells[i]->type == LVAL_SYM,
            "Expected parameter %lu to be of type '%s'; got type '%s'.", i + 1, ltype_name(LVAL_SYM), ltype_name(formals->val.l.cells[i]->type));
    }

    formals = lval_pop(v, 0);
    lval_t *name = lval_pop(formals, 0);

    body = lval_pop(v, 0);
    lval_t *fun = lval_lambda(formals, body);

    lenv_put(e, name, fun);
    lval_del(fun);
    lval_del(name);
    lval_del(v);

    return lval_sexpr();
}

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
    LEXACT_ARGS(v, sym, 2);
    LARG_TYPE(v, sym, 0, LVAL_INT);
    LARG_TYPE(v, sym, 1, LVAL_INT);

    lval_t *lhs = v->val.l.cells[0];
    lval_t *rhs = v->val.l.cells[1];

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
    return lval_int(res);
}

lval_t *builtin_cmp(lenv_t *e, lval_t *v, char *sym) {
    UNUSED(e);
    LEXACT_ARGS(v, sym, 2);

    lval_t *lhs = v->val.l.cells[0];
    lval_t *rhs = v->val.l.cells[1];

    long long res = 0;
    if ( strcmp(sym, "==") == 0 ) {
        res = lval_eq(lhs, rhs);
    } else if ( strcmp(sym, "!=") == 0 ) {
        res = !lval_eq(lhs, rhs);
    }

    lval_del(v);
    return lval_int(res);
}

lval_t *builtin_eq(lenv_t *e, lval_t *v) {
    return builtin_cmp(e, v, "==");
}

lval_t *builtin_ne(lenv_t *e, lval_t *v) {
    return builtin_cmp(e, v, "!=");
}

lval_t *builtin_if(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, "if", 3);
    LARG_TYPE(v, "if", 0, LVAL_INT);
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

lval_t *lval_call(lenv_t *e, lval_t *f, lval_t *v) {

    if ( f->type == LVAL_BUILTIN ) {
        return f->val.builtin(e, v);
    }

    lfunc_t *func = f->val.fun;
    lval_t **args = v->val.l.cells;
    lval_t *formals = func->formals;

    size_t given = v->val.l.count;
    size_t total = formals->val.l.count;

    while ( v->val.l.count > 0 ) {
 
        if ( formals->val.l.count == 0 &&
            args[0]->type != LVAL_SEXPR ) {
            /* Error case: non-symbolic parameter parsed */
            lval_del(v);
            return lval_err(
                "Function passed too many arguments; "
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

        lval_t *sym = lval_pop(formals, 0); /* unbound name */

        if ( strcmp(sym->val.sym, "&") == 0 ) {
            /* Variable argument case with '&' */
            if ( formals->val.l.count != 1 ) {
                lval_del(v);
                return lval_err(
                    "Function format invalid. "
                    "Symbol '&' not followed by a single symbol."
                );
            }

            /* Binding rest of the arguments to nsym */
            lval_t *nsym = lval_pop(formals, 0);
            lenv_put(func->env, nsym, builtin_list(e, v));
            lval_del(sym);
            lval_del(nsym);
            break;
        }

        lval_t *val = lval_pop(v, 0); /* value to apply to unbound name */

        lenv_put(func->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    lval_del(v);

    if ( formals->val.l.count > 0 &&
        strcmp(formals->val.l.cells[0]->val.sym, "&") == 0 ) {
        /* only first non-variable arguments was applied; create a empty qexpr  */

        if ( formals->val.l.count != 2 ) {
            return lval_err(
                    "Function format invalid. "
                    "Symbol '&' not followed by single symbol."
                    );
        }

        /* Remove '&' */
        lval_del(lval_pop(formals, 0));

        /* Create a empty list for the next symbol */
        lval_t *sym = lval_pop(formals, 0);
        lval_t *val = lval_qexpr();

        lenv_put(func->env, sym, val);
        lval_del(sym);
        lval_del(val);
    }

    if ( formals->val.l.count == 0 ) {
        func->env->parent = e;
        return builtin_eval( func->env, lval_add(lval_sexpr(), lval_copy(func->body)) );
    }
    return lval_copy(f);
}


lval_t *lval_eval_sexpr(lenv_t *e, lval_t *v) {

    /* depth-first eval of sexpr */
    for ( size_t i = 0; i < v->val.l.count; i++ ) {
        v->val.l.cells[i] = lval_eval(e, v->val.l.cells[i]);
    }

    /* return first error */
    for ( size_t i = 0; i < v->val.l.count; i++ ) {
        if ( v->val.l.cells[i]->type == LVAL_ERR ) {
            return lval_take(v, i);
        }
    }

    /* empty sexpr */
    if ( v->val.l.count == 0 ) {
        return v;
    }

    /* hoist first lval if only one is available */
    if ( v->val.l.count == 1 ) {
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

