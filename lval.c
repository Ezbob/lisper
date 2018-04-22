#include "lval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

lval_t *lval_num(double num) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_NUM;
    val->val.num = num;
    return val;
}

lval_t *lval_err(char *err) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_ERR;
    val->val.err = malloc(strlen(err) + 1);
    strcpy(val->val.err, err);
    return val;
}

lval_t *lval_sym(char* sym) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_SYM;
    val->val.sym = malloc(strlen(sym) + 1);
    strcpy(val->val.sym, sym);
    return val;
}

lval_t *lval_sexpr(void) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_SEXPR;
    val->val.symcells = malloc(sizeof(lscell_t));
    val->val.symcells->count = 0;
    val->val.symcells->cells = NULL;
    return val;
}

void lval_destroy(lval_t *val) {
    switch (val->type) {
        case LVAL_NUM: 
            break;
        case LVAL_ERR:
            free(val->val.err);
            break;
        case LVAL_SYM:
            free(val->val.sym);
            break;
        case LVAL_SEXPR:
            for ( size_t i = 0; i < val->val.symcells->count; i++ ) {
                lval_destroy(val->val.symcells->cells[i]);
            }
            free(val->val.symcells);
            break;
    }
    free(val);
}

void lval_expr_print(lval_t *val, char prefix, char suffix) {
    putchar(prefix);
    
    for ( size_t i = 0; i < val->val.symcells->count; i++ ) {
        lval_print(val->val.symcells->cells[i]);

        if ( i != ( val->val.symcells->count - 1 ) ) {
            putchar(' ');
        }
    }

    putchar(suffix);
}

void lval_print(lval_t *val) {

    switch ( val->type ) {
        case LVAL_NUM:
            printf("%lf", val->val.num);
            break;
        case LVAL_ERR:
            printf("Error: %s", val->val.err);
            break;
        case LVAL_SYM:
            printf("%s", val->val.sym);
            break;
        case LVAL_SEXPR:
            lval_expr_print(val, '(', ')');
            break;
    }
}

void lval_println(lval_t *val) {
    lval_print(val);
    putchar('\n');
}

lval_t *lval_read_num(mpc_ast_t *t) {
        double num = 0.0;
        int code = sscanf(t->contents, "%lf", &num);

        if ( code == 1 ) {
            return lval_num(num);
        } else {
            return lval_err("Invalid number");
        }
}

lval_t *lval_add(lval_t *val, lval_t *other) {
    lscell_t *cells = val->val.symcells;
    cells->count++;
    cells->cells = realloc(cells->cells, sizeof(lval_t *) * cells->count);
    cells->cells[cells->count - 1] = other;
    return val;
}

lval_t *lval_read(mpc_ast_t *t) {
    lval_t *val = NULL;

    if ( strstr(t->tag, "number") ) {
        return lval_read_num(t);
    }

    if ( strstr(t->tag, "symbol") ) {
        return lval_sym(t->contents);
    }

    if ( strcmp(t->tag, ">") == 0 || strstr(t->tag, "sexpr") ) {
        /* ">" is the root of the AST */
        val = lval_sexpr();
    }

    for ( int i = 0; i < t->children_num; i++ ) {
        if ( strcmp(t->children[i]->contents, "(") == 0 ||
             strcmp(t->children[i]->contents, ")") == 0 ||
             strcmp(t->children[i]->tag, "regex") == 0 ) {
            continue;
        }
        val = lval_add(val, lval_read(t->children[i]));
    }

    return val;
}

lval_t *lval_pop(lval_t *v, int i) {
    lval_t *x = v->val.symcells->cells[i];

    memmove(&v->val.symcells->cells[i], &v->val.symcells->cells[i + 1], sizeof(lval_t *) * (v->val.symcells->count - i - 1));

    v->val.symcells->count--;

    v->val.symcells->cells = realloc(v->val.symcells->cells, sizeof(lval_t *) * v->val.symcells->count);
    
    return x;
}

lval_t *lval_take(lval_t *v, int i) {
    lval_t *x = lval_pop(v, i);
    lval_destroy(v);
    return x;
}

lval_t *builtin_eval(lval_t *v, char *sym) {
    lscell_t *c = v->val.symcells;

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

    while ( v->val.symcells->count > 0 ) {
        
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

lval_t *lval_eval_sexpr(lval_t *v) {
    lscell_t *symc;
    
    /* depth-first eval of sexpr */
    symc = v->val.symcells;
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
    lval_t *res = builtin_eval(v, f->val.sym);
    lval_destroy(f);
    return res;
}

lval_t *lval_eval(lval_t *v) {
    if ( v->type == LVAL_SEXPR ) {
        return lval_eval_sexpr(v);
    }
    return v;
}

