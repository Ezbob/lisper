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

void lval_destroy(lval_t *val) {
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
            for ( size_t i = 0; i < val->val.l->count; i++ ) {
                lval_destroy(val->val.l->cells[i]);
            }
            free(val->val.l);
            break;
    }
    free(val);
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
        return lval_err("Invalid number");
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
        printf("Fatal memory error when trying reallocating for offer. Stopping.");
        lval_destroy(val);
        exit(1);
    }
    val->val.l->cells = resized;
    memmove(val->val.l->cells + 1, val->val.l->cells, sizeof(lval_t *) * ( val->val.l->count - 1 ) );
        // move memory at address val->val.l->cells (op to old count of cells) to addr val->val.l->cells[1]

    val->val.l->cells[0] = other;
        // insert into the front of the array

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

    memmove(&v->val.l->cells[i], &v->val.l->cells[i + 1], sizeof(lval_t *) * (v->val.l->count - i - 1));

    v->val.l->count--;
    lval_t **realloced = realloc(v->val.l->cells, sizeof(lval_t *) * v->val.l->count);
    if ( realloced == NULL && v->val.l->count != 0 ) {
        // since we always shrink the array we will hit count == 0 eventually,
        // making the reallocated pointer some value that shouldn't be dereferenced
        printf("Fatal memory error when trying reallocating for pop. Stopping.");
        lval_destroy(v);
        exit(1);
    }
    v->val.l->cells = realloced;

    return x;
}

lval_t *lval_take(lval_t *v, int i) {
    lval_t *x = lval_pop(v, i);
    lval_destroy(v);
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
            x->val.err = malloc((strlen(v->val.err) + 1) * sizeof(char)); /* copy */
            strcpy(x->val.err, v->val.err);
            break;
        case LVAL_SYM:
            x->val.sym = malloc((strlen(v->val.sym) + 1) * sizeof(char));
            strcpy(x->val.sym, v->val.sym);
            break;
        case LVAL_SEXPR:
        case LVAL_QEXPR:
            x->val.l->count = v->val.l->count;
            x->val.l->cells = malloc(sizeof(lval_t *) * x->val.l->count);
            for ( size_t i = 0; i < x->val.l->count; ++i ) {
                x->val.l->cells[i] = lval_copy(v->val.l->cells[i]);
            }
            break;
    }

    return x;
}

