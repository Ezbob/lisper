#include "builtin.h"
#include "lval.h"
#include "lenv.h"

#define UNUSED(x) (void)(x)

#define LASSERT(args, cond, err) \
    if ( !(cond) ) { lval_destroy(args); return lval_err(err);  }

#define LLEAST_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l->count >= numargs, "Not enough arguments parsed to '"#funcname"'. Expected at least "#numargs" argument(s).")

#define LMOST_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l->count <= numargs, "Too many arguments parsed to '"#funcname"'. Expected at most "#numargs" argument(s).")

#define LEXACT_ARGS(sym, funcname, numargs) \
    LASSERT(sym, sym->val.l->count == numargs, "Wrong number of arguments parsed to '"#funcname"'. Expected at exactly "#numargs" argument(s). ")

#define LNOT_EMPTY_QEXPR(sym, funcname, i) \
    LASSERT(sym, sym->val.l->cells[i]->type == LVAL_QEXPR && sym->val.l->cells[i]->val.l->count > 0, "Empty qexpression parsed to '"#funcname"'.")


lval_t *builtin_op(lenv_t *e, lval_t *v, char *sym) {
    UNUSED(e);
    lcell_list_t *c = v->val.l;

    for ( size_t i = 0; i < c->count; i++ ) {
        if ( c->cells[i]->type != LVAL_NUM ) {
            lval_destroy(v);
            return lval_err("Cannot operate on non-number");
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
                lval_destroy(a); 
                lval_destroy(b);
                a = lval_err("Division by zero");
                break;
            }
            a->val.num /= b->val.num;
        } else if ( strcmp(sym, "%") == 0 ) {
            if ( b->val.num == 0 ) {
                lval_destroy(a); 
                lval_destroy(b);
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
        lval_destroy(b);
    }

    lval_destroy(v);
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

    LEXACT_ARGS(v, tail, 1);
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Wrong type of argument parsed to 'tail'. 'tail' expects a qexpression. ");

    lval_t *a = lval_take(v, 0);
    if ( a->val.l->count > 0 ) {
        lval_destroy(lval_pop(a, 0));
    }
    return a;
}

lval_t *builtin_head(lenv_t *e, lval_t *v) {
    UNUSED(e);

    LEXACT_ARGS(v, head, 1);
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Wrong type of argument parsed to 'head'. 'head' applies to qexpressions." );
        
    lval_t *a = lval_take(v, 0);
    
    while ( a->val.l->count > 1 ) {
        lval_destroy(lval_pop(a, 1));
    }

    return a;
}

lval_t *builtin_list(lenv_t *e, lval_t *v) {
    UNUSED(e);
    v->type = LVAL_QEXPR;
    return v;
}

lval_t *builtin_eval(lenv_t *e, lval_t *v) {
    LEXACT_ARGS(v, eval, 1);
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Incorrect type of argument parsed to 'eval'");
    
    lval_t *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(e, a);
}

lval_t *lval_join(lval_t *x, lval_t *y) {
    while ( y->val.l->count ) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_destroy(y);
    return x;
}

lval_t *builtin_join(lenv_t *e, lval_t *v) {
    UNUSED(e);
    for ( size_t i = 0; i < v->val.l->count; ++i ) {
        LASSERT(v, v->val.l->cells[i]->type == LVAL_QEXPR,
            "Incorrect type of argument parsed to 'join'");
    }

    lval_t *a = lval_pop(v, 0);
    while ( v->val.l->count ) {
        a = lval_join(a, lval_pop(v, 0));
    }

    lval_destroy(v);

    return a;
}

lval_t *builtin_cons(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, cons, 2);
    LASSERT(v, v->val.l->cells[1]->type == LVAL_QEXPR, "Wrong argument type. Second argument should be a qexpression.");

    lval_t *otherval = lval_pop(v, 0);
    lval_t *qexpr = lval_pop(v, 0);

    lval_offer(qexpr, otherval);

    lval_destroy(v);
    return qexpr;
}

lval_t *builtin_len(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, len, 1);
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Wrong type of argument parsed to 'len'." );

    lval_t *arg = lval_pop(v, 0);
    size_t count = arg->val.l->count;

    while ( arg->val.l->count ) {
        lval_destroy(lval_pop(arg, 0));
    }

    arg->type = LVAL_NUM;
    arg->val.num = ( (double) count ); // we only have double defined as numbers
    lval_destroy(v);

    return arg;
}

lval_t *builtin_init(lenv_t *e, lval_t *v) {
    UNUSED(e);
    LEXACT_ARGS(v, init, 1);
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Wrong type of argument parsed to 'init'.");

    lval_t *qexpr = lval_pop(v, 0);
    if ( qexpr->val.l->count > 0 ) {
        lval_destroy(lval_pop(qexpr, (qexpr->val.l->count - 1) ));
    }
    lval_destroy(v);
    return qexpr;
}

lval_t *builtin_def(lenv_t *e, lval_t *v) {
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Wrong type of argument parsed to 'def'.");

    lval_t *syms = v->val.l->cells[0];

    for ( size_t i = 0; i < syms->val.l->count; ++i ) {
        LASSERT(v, syms->val.l->cells[i]->type == LVAL_SYM, "Function 'def' cannot assign value(s) to name(s). One of the names contains non-symbols.");
    }

    LASSERT(v, syms->val.l->count == v->val.l->count - 1, "Function 'def' cannot assign value(s) to name(s). Number of name(s) and value(s) does not match.");

    for ( size_t i = 0; i < syms->val.l->count; ++i ) {
        lenv_put(e, syms->val.l->cells[i], v->val.l->cells[i + 1]);
    }

    lval_destroy(v);
    return lval_sexpr();
}

lval_t *lval_eval_sexpr(lenv_t *e, lval_t *v) {
    lcell_list_t *symc;
    
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

    /* Symbolic expression was not defined by a symbol */
    lval_t *f = lval_pop(v, 0);
    if ( f->type != LVAL_FUN ) {
        lval_destroy(f);
        lval_destroy(v);
        return lval_err("First element in S-expression is not a function");
    }

    /* Using builtins to compute expressions */
    lval_t *res = f->val.fun(e, v);
    lval_destroy(f);
    return res;
}

lval_t *lval_eval(lenv_t *e, lval_t *v) {
    lval_t *x; 
    switch ( v->type ) {
        case LVAL_SYM:
            x = lenv_get(e, v);
            lval_destroy(v);
            return x;

        case LVAL_SEXPR:
            return lval_eval_sexpr(e, v);
        default:
            break;
    }

    return v;
}

