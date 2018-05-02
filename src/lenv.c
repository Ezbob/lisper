#include "lenv.h"
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "lval.h"

#define LENV_BUILTIN(name) lenv_add_builtin(e, #name, builtin_##name)
#define LENV_SYMBUILTIN(sym, name) lenv_add_builtin(e, sym, builtin_##name)

lenv_t *lenv_new(void) {
    lenv_t *env = malloc(sizeof(lenv_t));
    env->parent = NULL;
    env->count = 0;
    env->syms = NULL;
    env->vals = NULL;
    return env;
}

void lenv_del(lenv_t *env) {
    env->parent = NULL;
    for ( size_t i = 0; i < env->count; ++i ) {
        free(env->syms[i]);
        lval_del(env->vals[i]);
    }
    free(env->syms);
    free(env->vals);
    free(env);
}

lenv_t *lenv_copy(lenv_t *env) {
    lenv_t *new = lenv_new();

    new->parent = env->parent;
    new->count = env->count;
    new->syms = malloc(new->count * sizeof(char *));
    new->vals = malloc(new->count * sizeof(lval_t *));

    for ( size_t i = 0; i < new->count; ++i ) {
        new->syms[i] = calloc(strlen(env->syms[i]) + 1, sizeof(char));
        strcpy(new->syms[i], env->syms[i]);
        new->vals[i] = lval_copy(env->vals[i]);
    }

    return new;
}

void lenv_add_builtin(lenv_t *e, char *name, lbuiltin func) {
    lval_t *k = lval_sym(name);
    lval_t *v = lval_builtin(func);

    lenv_put(e, k, v);

    lval_del(k);
    lval_del(v);
}

void lenv_add_builtins(lenv_t *e) {
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
    LENV_BUILTIN(fun);
    LENV_BUILTIN(if);

    LENV_SYMBUILTIN("+", add);
    LENV_SYMBUILTIN("-", sub);
    LENV_SYMBUILTIN("*", mul);
    LENV_SYMBUILTIN("/", div);
    LENV_SYMBUILTIN("%", fmod);
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

lval_t *lenv_get(lenv_t *e, lval_t *k) {

    for ( size_t i = 0; i < e->count; ++i ) {
        if ( strcmp(e->syms[i], k->val.sym) == 0 ) {
            return lval_copy(e->vals[i]);
        }
    }

    if ( e->parent != NULL ) {
        return lenv_get(e->parent, k);
    }

    return lval_err("Unbound symbol '%s'", k->val.sym);
}

void lenv_put(lenv_t *e, lval_t *k, lval_t *v) {

    for ( size_t i = 0; i < e->count; ++i ) {
        /* linear search for sym name */
        if ( strcmp(e->syms[i], k->val.sym) == 0 ) {
            lval_del(e->vals[i]);
            e->vals[i] = lval_copy(v); /* override already existing value */
            return;
        }
    }

    e->count++;
    e->vals = realloc(e->vals, sizeof(lval_t *) * e->count);
    e->syms = realloc(e->syms, sizeof(lval_t *) * e->count); /* FIXME probably check for NULL  */

    e->vals[e->count - 1] = lval_copy(v);
    e->syms[e->count - 1] = malloc(strlen(k->val.sym) + 1);
    strcpy(e->syms[e->count - 1], k->val.sym);

}

void lenv_def(lenv_t *e, lval_t *k, lval_t *v) {

    while ( e->parent != NULL ) {
        e = e->parent;
    }
    lenv_put(e, k, v);
}

void lenv_pretty_print(lenv_t *e) {
    printf("DEBUG -- lenv content:\n");
    for ( size_t i = 0; i < e->count; ++i ) {
        printf("    n: %s t: %s p: %p\n", e->syms[i], ltype_name(e->vals[i]->type), (void *) (e->vals + i));
    }
    printf("DEBUG -- lenv END\n");
}
