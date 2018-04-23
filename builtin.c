#include "builtin.h"
#include "lval.h"


lval_t *builtin_op(lval_t *v, char *sym) {
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

lval_t *builtin_tail(lval_t *v) {

    LASSERT(v, v->val.l->count == 1, "Too many arguments parsed to 'tail'");
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Wrong type of argument parsed to 'tail'. 'tail' expects a qexpression. ");
    LASSERT(v, v->val.l->count != 0, "Empty qexpression parsed to 'tail'");

    lval_t *a = lval_take(v, 0);
    lval_destroy(lval_pop(v, 0));

    return a;
}

lval_t *builtin_head(lval_t *v) {

    LASSERT(v, v->val.l->count == 1, "Too many arguments parsed to 'head'");
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Wrong type of argument parsed to 'head'. 'head' applies to qexpressions." );
    LASSERT(v, v->val.l->count != 0, "Empty qexpression parsed to 'head'");
        
    lval_t *a = lval_take(v, 0);
    
    while ( a->val.l->count > 1 ) {
        lval_destroy(lval_pop(a, 1));
    }

    return a;
}

lval_t *builtin_list(lval_t *v) {
    v->type = LVAL_QEXPR;
    return v;
}

lval_t *builtin_eval(lval_t *v) {
    LASSERT(v, v->val.l->count == 1, "Too many arguments parsed to 'eval'." );
    LASSERT(v, v->val.l->cells[0]->type == LVAL_QEXPR, "Incorrect type of argument parsed to 'eval'");
    
    lval_t *a = lval_take(v, 0);
    a->type = LVAL_SEXPR;
    return lval_eval(a);
}

lval_t *lval_join(lval_t *x, lval_t *y) {
    while ( y->val.l->count ) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_destroy(y);
    return x;
}

lval_t *builtin_join(lval_t *v) {

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

lval_t *builtin(lval_t *a, char *func) {
    if ( strcmp("list", func) == 0 ) { return builtin_list(a); }
    if ( strcmp("head", func) == 0 ) { return builtin_head(a); }
    if ( strcmp("tail", func) == 0 ) { return builtin_tail(a); }
    if ( strcmp("join", func) == 0 ) { return builtin_join(a); }
    if ( strcmp("eval", func) == 0 ) { return builtin_eval(a); }

    if ( strcmp("max", func) == 0 ) { return builtin_op(a, func); }
    if ( strcmp("min", func) == 0 ) { return builtin_op(a, func); }

    if ( strstr("+-/*^%", func) ) {
        return builtin_op(a, func);
    }

    lval_destroy(a);

    return lval_err("Unknown function");
}


lval_t *lval_eval_sexpr(lval_t *v) {
    lcell_list_t *symc;
    
    /* depth-first eval of sexpr */
    symc = v->val.l;
    for ( size_t i = 0; i < symc->count; i++ ) {
        symc->cells[i] = lval_eval(symc->cells[i]); 
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
    if ( f->type != LVAL_SYM ) {
        lval_destroy(f);
        lval_destroy(v);
        return lval_err("S-expression does not start with a symbol");
    }

    /* Using builtins to compute expressions */
    lval_t *res = builtin(v, f->val.sym);
    lval_destroy(f);
    return res;
}

lval_t *lval_eval(lval_t *v) {
    if ( v->type == LVAL_SEXPR ) {
        return lval_eval_sexpr(v);
    }
    return v;
}

