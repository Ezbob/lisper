#include "lval.h"
#include "lenv.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

lval_t *lval_int(long long num) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_INT;
    val->val.intval = num;
    return val;
}

lval_t *lval_float(double num) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_FLOAT;
    val->val.floatval = num;
    return val;
}

lval_t *lval_bool(long long num) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_BOOL;
    val->val.intval = num;
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
    val->val.l.count = 0;
    val->val.l.cells = NULL;
    return val;
}

lval_t *lval_qexpr(void) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_QEXPR;
    val->val.l.count = 0;
    val->val.l.cells = NULL;
    return val;
}

lval_t *lval_str(char *s) {
    lval_t *v = malloc(sizeof(lval_t));
    v->type = LVAL_STR;
    v->val.str = malloc(strlen(s) + 1);
    strcpy(v->val.str, s);
    return v;
}

lval_t *lval_builtin(lbuiltin f) {
    lval_t *val = malloc(sizeof(lval_t));
    val->type = LVAL_BUILTIN;
    val->val.builtin = f;
    return val;
}

lfunc_t *lfunc_new(lenv_t *env, lval_t *formals, lval_t *body) {
    lfunc_t *new = malloc(sizeof(lfunc_t));
    new->env = env;
    new->formals = formals;
    new->body = body;
    return new;
}

lval_t *lval_lambda(lval_t *formals, lval_t *body) {
    lval_t *nw = malloc(sizeof(lval_t));
    nw->type = LVAL_LAMBDA;
    nw->val.fun = lfunc_new(lenv_new(), formals, body);
    return nw;
}

void lfunc_del(lfunc_t *);

void lval_del(lval_t *val) {
    switch (val->type) {
        case LVAL_FLOAT:
        case LVAL_INT:
        case LVAL_BUILTIN:
        case LVAL_BOOL:
            break;
        case LVAL_LAMBDA:
            lfunc_del(val->val.fun);
            break;
        case LVAL_ERR:
            free(val->val.err);
            break;
        case LVAL_SYM:
            free(val->val.sym);
            break;
        case LVAL_STR:
            free(val->val.str);
            break;
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for ( size_t i = 0; i < val->val.l.count; ++i ) {
                lval_del(val->val.l.cells[i]);
            }
            break;
    }
    free(val);
}

void lfunc_del(lfunc_t *f) {
    lenv_del(f->env);
    lval_del(f->formals);
    lval_del(f->body);
    free(f);
}

void lval_expr_print(lval_t *val, char prefix, char suffix) {
    putchar(prefix);
    
    for ( size_t i = 0; i < val->val.l.count; i++ ) {
        lval_print(val->val.l.cells[i]);

        if ( i != ( val->val.l.count - 1 ) ) {
            putchar(' ');
        }
    }

    putchar(suffix);
}

void lval_print_str(lval_t *v) {

    char *escaped = malloc(strlen(v->val.str) + 1);
    strcpy(escaped, v->val.str);

    escaped = mpcf_escape(escaped);
    printf("\"%s\"", escaped);

    free(escaped);
}

void lval_print(lval_t *val) {
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
            printf("Error: %s", val->val.err);
            break;
        case LVAL_SYM:
            printf("%s", val->val.sym);
            break;
        case LVAL_STR:
            lval_print_str(val);
            break;
        case LVAL_SEXPR:
            lval_expr_print(val, '(', ')');
            break;
        case LVAL_QEXPR:
            lval_expr_print(val, '{', '}');
            break;
        case LVAL_LAMBDA:
            printf("(\\ ");
            lval_print(val->val.fun->formals);
            putchar(' ');
            lval_print(val->val.fun->body);
            putchar(')');
            break;
        case LVAL_BUILTIN:
            printf("<builtin>");
            break;
    }
}

void lval_println(lval_t *val) {
    lval_print(val);
    putchar('\n');
}

enum { 
    LREAD_FLOAT = 0,
    LREAD_INT = 1
};

lval_t *lval_read_num(mpc_ast_t *t, int choice) {

    int code = 0;
    long long int int_read = 0;
    double float_read = 0.0;

    switch (choice) {
        case LREAD_FLOAT:
            code = sscanf(t->contents, "%lf", &float_read);

            if ( code == 1 ) {
                return lval_float(float_read);
            }
            break;
        case LREAD_INT:
            code = sscanf(t->contents, "%lli", &int_read);

            if ( code == 1 ) {
                return lval_int(int_read);
            }
            break;
    }

    return lval_err("Cloud not parse '%s' as a number.", t->contents);
}

lval_t *lval_add(lval_t *val, lval_t *other) {
    val->val.l.count++;
    lval_t **resized_cells = realloc(val->val.l.cells, val->val.l.count * sizeof(lval_t *));
    if ( resized_cells == NULL ) {
        lval_del(val);
        lval_del(other);
        perror("Could not resize lval cell buffer"); 
        exit(1);
    }
    resized_cells[val->val.l.count - 1] = other;
    val->val.l.cells = resized_cells;
    return val;
}

lval_t *lval_offer(lval_t *val, lval_t *other) {
    val->val.l.count++;
    lval_t **resized = realloc(val->val.l.cells, val->val.l.count * sizeof(lval_t*));
        // resize the memory buffer to carry another cell

    if ( resized == NULL ) {
        perror("Fatal memory error when trying reallocating for offer");
        lval_del(val);
        exit(1);
    }
    val->val.l.cells = resized;
    memmove(val->val.l.cells + 1, val->val.l.cells, (val->val.l.count - 1) * sizeof(lval_t*));
        // move memory at address val->val.l.cells (op to old count of cells) to addr val->val.l.cells[1]

    val->val.l.cells[0] = other;
        // insert into the front of the array

    return val;
}

lval_t *lval_join(lval_t *x, lval_t *y) {
    while ( y->val.l.count ) {
        x = lval_add(x, lval_pop(y, 0));
    }

    lval_del(y);
    return x;
}

lval_t *lval_read_str(mpc_ast_t *t) {

    t->contents[strlen(t->contents) - 1] = '\0';

    char *unescaped = malloc(strlen(t->contents + 1) + 1);
    strcpy(unescaped, t->contents + 1);

    unescaped = mpcf_unescape(unescaped);
    lval_t *str = lval_str(unescaped);

    free(unescaped);
    return str;
}

lval_t *lval_read(mpc_ast_t *t) {
    lval_t *val = NULL;

    if ( strstr(t->tag, "boolean") ) {
        long long res = (strcmp(t->contents, "true") == 0) ? 1 : 0;
        return lval_bool(res);
    }

    if ( strstr(t->tag, "string") ) {
        return lval_read_str(t);
    }

    if ( strstr(t->tag, "float") ) {
        return lval_read_num(t, LREAD_FLOAT);
    }

    if ( strstr(t->tag, "integer") ) {
        return lval_read_num(t, LREAD_INT);
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
    lval_t *x = v->val.l.cells[i];
    memmove(v->val.l.cells + i, v->val.l.cells + (i + 1), sizeof(lval_t *) * (v->val.l.count - i - 1));
    v->val.l.count--;

    lval_t **cs = realloc(v->val.l.cells, v->val.l.count * sizeof(lval_t *));
    if ( !cs && v->val.l.count > 0 ) {
        perror("Could not shrink cell buffer");
        lval_del(v);
        exit(1);
    }
    v->val.l.cells = cs;

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
        case LVAL_LAMBDA:
            x->val.fun = lfunc_new(
                lenv_copy(v->val.fun->env),
                lval_copy(v->val.fun->formals),
                lval_copy(v->val.fun->body)
            );
            break;
        case LVAL_BUILTIN:
            x->val.builtin = v->val.builtin;
            break;
        case LVAL_FLOAT:
            x->val.floatval = v->val.floatval;
            break;
        case LVAL_STR:
            x->val.str = malloc(strlen(v->val.str) + 1);
            strcpy(x->val.str, v->val.str);
            break;
        case LVAL_INT:
        case LVAL_BOOL:
            x->val.intval = v->val.intval;
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
            x->val.l.count = v->val.l.count;
            x->val.l.cells = malloc(v->val.l.count * sizeof(lval_t *));
            for ( size_t i = 0; i < x->val.l.count; ++i ) {
                x->val.l.cells[i] = lval_copy(v->val.l.cells[i]);
            }
            break;
    }

    return x;
}

int lval_eq(lval_t *x, lval_t *y) {
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
            return (strcmp(x->val.err, y->val.err) == 0);
        case LVAL_SYM:
            return (strcmp(x->val.sym, y->val.sym) == 0);
        case LVAL_BUILTIN:
            return (x->val.builtin == y->val.builtin);
        case LVAL_LAMBDA:
            return lval_eq(x->val.fun->formals, y->val.fun->formals) &&
                 lval_eq(x->val.fun->body, y->val.fun->body);
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            if ( x->val.l.count != y->val.l.count ) {
                return 0;
            }
            for ( size_t i = 0; i < x->val.l.count; ++i ) {
                if ( !lval_eq(x->val.l.cells[i], y->val.l.cells[i]) ) {
                    return 0;
                }
            }
            return 1;
        case LVAL_STR:
            return strcmp(x->val.str, x->val.str) == 0;
    }
    return 0;
}

char *ltype_name(ltype t) {
    switch ( t ) {
        case LVAL_SYM:
            return "symbol";
        case LVAL_BUILTIN:
            return "builtin";
        case LVAL_LAMBDA:
            return "lambda";
        case LVAL_FLOAT:
            return "float";
        case LVAL_BOOL:
            return "boolean";
        case LVAL_INT:
            return "integer";
        case LVAL_STR:
            return "string";
        case LVAL_QEXPR:
            return "q-expression";
        case LVAL_SEXPR:
            return "s-expression";
        case LVAL_ERR:
            return "error";
        default:
            break;
    }
    return "Unknown";
}

void lval_depth_print(lval_t *v, size_t depth) {
    
    for ( size_t i = 0; i < depth; ++i ) {
        putchar(' ');
    }

    printf("`-t: %s = ", ltype_name(v->type));

    lval_println(v);

    switch ( v->type ) {
        case LVAL_QEXPR:
        case LVAL_SEXPR:
            for ( size_t i = 0; i < v->val.l.count; ++i ) {
                lval_depth_print(v->val.l.cells[i], depth + 1);
            }
            break;
        default:
            break;
    }

}

void lval_pretty_print(lval_t *v) {
    lval_depth_print(v, 0);
}

