#include "lenv.h"
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "lval.h"
#include "hashmap.h"


lenv_t *lenv_new(void) {
    lenv_t *env = malloc(sizeof(lenv_t));
    env->parent = NULL;
    env->map = HM_initialize_hashmap(500);
    return env;
}

lenv_t *lenv_new_nomap(void) {
    lenv_t *env = malloc(sizeof(lenv_t));
    env->parent = NULL;
    env->map = NULL;
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
    lenv_t *new = lenv_new_nomap();
    new->parent = env->parent;
    new->map = HM_copyHashmap(env->map);
    return new;
}

void lenv_add_builtin(lenv_t *e, char *name, lbuiltin func) {
    lval_t *k = lval_sym(name);
    lval_t *v = lval_builtin(func);

    lenv_put(e, k, v);

    lval_del(k);
    lval_del(v);
}

lval_t *lenv_get(lenv_t *e, lval_t *k) {

    HM_VALUE *val = HM_getValue(e->map, k->val.strval);

    if ( val != NULL ) {
        /* found */
        return lval_copy((lval_t *) val->value);
    } else if ( e->parent != NULL ) {
        return lenv_get(e->parent, k);
    }

    return lval_err("Unbound symbol '%s'", k->val.strval);
}

void lenv_put(lenv_t *e, lval_t *k, lval_t *v) {
    HM_putValue(e->map, k->val.strval, v);
}

void lenv_def(lenv_t *e, lval_t *k, lval_t *v) {

    while ( e->parent != NULL ) {
        e = e->parent;
    }
    lenv_put(e, k, v);
}

void lenv_pretty_print(lenv_t *e) {
    printf("DEBUG -- lenv content:\n");
    /* TODO pretty print of map? */
    /*for ( size_t i = 0; i < e->count; ++i ) {
        printf("    n: %s t: %s p: %p\n", e->syms[i], ltype_name(e->vals[i]->type), (void *) (e->vals + i));
    }*/
    printf("DEBUG -- lenv END\n");
}
