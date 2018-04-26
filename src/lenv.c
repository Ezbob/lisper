#include "lenv.h"
#include <stdlib.h>
#include "builtin.h"

#define LENV_BUILTIN(name) lenv_add_builtin(e, #name, builtin_##name)
#define LENV_SYMBUILTIN(sym, name) lenv_add_builtin(e, #sym, builtin_##name)

lenv_t *lenv_new(void) {
    lenv_t *env = malloc(sizeof(lenv_t));
    env->count = 0;
    env->syms = NULL;
    env->vals = NULL;
    return env;
}

void lenv_destroy(lenv_t *env) {
    for (size_t i = 0; i < env->count; ++i) {
        free(env->syms[i]);
        lval_destroy(env->vals[i]);
    }

    free(env->syms);
    free(env->vals);
    free(env);
}

void lenv_add_builtin(lenv_t *e, char *name, lbuiltin func) {
    lval_t *k = lval_sym(name);
    lval_t *v = lval_fun(func);

    lenv_put(e, k, v);

    lval_destroy(k);
    lval_destroy(v);
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

    LENV_BUILTIN(max);
    LENV_BUILTIN(min);

    LENV_SYMBUILTIN(+, add);
    LENV_SYMBUILTIN(-, sub);
    LENV_SYMBUILTIN(*, mul);
    LENV_SYMBUILTIN(/, div);
    LENV_SYMBUILTIN(%, fmod);
    LENV_SYMBUILTIN(^, pow);
}

lval_t *lenv_get(lenv_t *e, lval_t *k) {

    for ( size_t i = 0; i < e->count; ++i ) {
        if ( strcmp(e->syms[i], k->val.sym) == 0 ) {
            return lval_copy(e->vals[i]);
        }
    }
    return lval_err("unbound symbol");
}

void lenv_put(lenv_t *e, lval_t *k, lval_t *v) {

    for ( size_t i = 0; i < e->count; ++i ) {
        if ( strcmp(e->syms[i], k->val.sym) == 0 ) {
            lval_destroy(e->vals[i]);
            e->vals[i] = lval_copy(v);
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

