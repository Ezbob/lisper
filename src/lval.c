#include "lval.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

lcell_list_t *lcell_list_new(void) {
    lcell_list_t *l = malloc(sizeof(lcell_list_t));;
    l->count = 0;
    l->cells = NULL;
    return l;
}

lval_t *lval_num(double num) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_NUM;
    val->val.num = num;
    return val;
}

lval_t *lval_err(char *fmt, ...) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_ERR;
    va_list va;
    va_start(va, fmt);

    val->val.err = malloc(512 * sizeof(char));
    vsnprintf(val->val.err, 511, fmt, va);

    val->val.err = realloc(val->val.err, strlen(val->val.err) + 1);

    va_end(va); 

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
    val->val.l = malloc(sizeof(lcell_list_t));
    val->val.l->count = 0;
    val->val.l->cells = NULL;
    return val;
}

lval_t *lval_qexpr(void) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_QEXPR;
    val->val.l = malloc(sizeof(lcell_list_t));
    val->val.l->count = 0;
    val->val.l->cells = NULL;
    return val;
}

lval_t *lval_fun(lbuiltin f) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_FUN;
    val->val.fun = f;
    return val;
}

void lcell_list_del(lcell_list_t *);

void lval_del(lval_t *val) {
    switch (val->type) {
        case LVAL_NUM: 
            break;
        case LVAL_FUN:
            break;
        case LVAL_ERR:
            free(val->val.err);
            break;
        case LVAL_SYM:
            free(val->val.sym);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            lcell_list_del(val->val.l);
            break;
    }
    free(val);
}

void lcell_list_del(lcell_list_t *l) {
    for (size_t i = 0; i < l->count; ++i) {
        lval_del(l->cells[i]);
    }
    free(l);
}

void lval_expr_print(lval_t *val, char prefix, char suffix) {
    putchar(prefix);
    
    for ( size_t i = 0; i < val->val.l->count; i++ ) {
        lval_print(val->val.l->cells[i]);

        if ( i != ( val->val.l->count - 1 ) ) {
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
        case LVAL_QEXPR:
            lval_expr_print(val, '{', '}');
            break;
        case LVAL_FUN:
            printf("<function>");
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
        return lval_err("Cloud not parse '%s' as a number.", t->contents);
    }
}

lval_t *lval_add(lval_t *val, lval_t *other) {
    val->val.l->count++;
    val->val.l->cells = realloc(val->val.l->cells, sizeof(lval_t *) * val->val.l->count);
    val->val.l->cells[val->val.l->count - 1] = other;
    return val;
}

lval_t *lval_offer(lval_t *val, lval_t *other) {
    val->val.l->count++;
    lval_t **resized = realloc(val->val.l->cells, sizeof(lval_t *) * val->val.l->count);
        // resize the memory buffer to carry another cell

    if ( resized == NULL ) {
        perror("Fatal memory error when trying reallocating for offer");
        lval_del(val);
        exit(1);
    }
    val->val.l->cells = resized;
    memmove(val->val.l->cells + 1, val->val.l->cells, sizeof(lval_t *) * ( val->val.l->count - 1 ) );
        // move memory at address val->val.l->cells (op to old count of cells) to addr val->val.l->cells[1]

    val->val.l->cells[0] = other;
        // insert into the front of the array

    return val;
}

lval_t *lval_join(lval_t *x, lval_t *y) {
    while ( y->val.l->count ) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
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

    if ( strstr(t->tag, "qexpr") ) {
        val = lval_qexpr();
    }

    for ( int i = 0; i < t->children_num; i++ ) {
        if ( strcmp(t->children[i]->contents, "(") == 0 ||
             strcmp(t->children[i]->contents, ")") == 0 ||
             strcmp(t->children[i]->contents, "{") == 0 ||
             strcmp(t->children[i]->contents, "}") == 0 ||
             strcmp(t->children[i]->tag, "regex") == 0 ) {
            continue;
        }
        val = lval_add(val, lval_read(t->children[i]));
    }

    return val;
}

lval_t *lval_pop(lval_t *v, int i) {
    lval_t *x = v->val.l->cells[i];
    memmove(v->val.l->cells + i, v->val.l->cells + (i + 1), sizeof(lval_t *) * (v->val.l->count - i - 1));
    v->val.l->count--;
    lval_t **cs = v->val.l->cells;
    cs = realloc(cs, sizeof(lval_t *) * v->val.l->count);
    if (!cs && v->val.l->count > 0) {
        perror("Could not shrink cell buffer");
        lval_del(v);
        exit(1);
    }
    v->val.l->cells = cs;

    return x;
}

lval_t *lval_take(lval_t *v, int i) {
    lval_t *x = lval_pop(v, i);
    lval_del(v);
    return x;
}

lval_t *lval_copy(lval_t *v) {

    lval_t *x = malloc(sizeof(lval_t));
    x->type = v->type;

    switch(v->type) {
        case LVAL_FUN: 
            x->val.fun = v->val.fun;
            break;
        case LVAL_NUM:
            x->val.num = v->val.num;
            break;
        case LVAL_ERR:
            x->val.err = calloc((strlen(v->val.err) + 1), sizeof(char));
            strcpy(x->val.err, v->val.err);
            break;
        case LVAL_SYM:
            x->val.sym = calloc((strlen(v->val.sym) + 1), sizeof(char));
            strcpy(x->val.sym, v->val.sym);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->val.l = lcell_list_new();
            x->val.l->count = v->val.l->count;
            x->val.l->cells = malloc(sizeof(lval_t *) * v->val.l->count);
            for ( size_t i = 0; i < x->val.l->count; ++i ) {
                x->val.l->cells[i] = lval_copy(v->val.l->cells[i]);
            }
            break;
    }

    return x;
}


char *ltype_name(ltype t) {
    switch ( t ) {
        case LVAL_SYM:
            return "symbol";
        case LVAL_FUN:
            return "function";
        case LVAL_NUM:
            return "number";
        case LVAL_QEXPR:
            return "q-expression";
        case LVAL_SEXPR:
            return "s-expression";
        default:
            break;
    }
    return "Unknown";
}

