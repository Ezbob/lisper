#include "lenv.h"
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "lval.h"

size_t lenv_hash( size_t capacity, char *k ) {
    size_t i;
    size_t res = 0;
    size_t k_size = strlen(k);

    for ( i = 0; i < k_size; ++i ) {
        res += k[i];
        if ( i < k_size - 1 ) {
            res <<= 1;
        }
    }

    return res % capacity;
}

lenv_t *lenv_new(size_t capacity) {
    lenv_t *env = malloc(sizeof(lenv_t));
    env->parent = NULL;
    env->entries = malloc(capacity * sizeof(lval_t *));
    env->capacity = capacity;
    for (size_t i = 0; i < capacity; ++i) {
        env->entries[i] = NULL;
    }
    return env;
}

void lenv_del(lenv_t *env) {
    env->parent = NULL;
    for ( size_t i = 0; i < env->capacity; ++i ) {
        if ( env->entries[i] != NULL ) {
            lval_del(env->entries[i]);
        }
    }
    free(env->entries);
    free(env);
}

lenv_t *lenv_copy(lenv_t *env) {
    lenv_t *new = lenv_new(env->capacity);
    new->parent = env->parent;
    for (size_t i = 0; i < env->capacity; ++i ) {
        if ( env->entries[i] != NULL ) {
            new->entries[i] = lval_copy(env->entries[i]);
        }
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

lval_t *lenv_get(lenv_t *e, lval_t *k) {

    size_t i = lenv_hash(e->capacity, k->val.strval);
    lval_t *val = e->entries[i];

    if ( val != NULL ) {
        /* found */
        return lval_copy(val);
    } else if ( e->parent != NULL ) {
        return lenv_get(e->parent, k);
    }

    return lval_err("Unbound symbol '%s'", k->val.strval);
}

void lenv_put(lenv_t *e, lval_t *k, lval_t *v) {
    size_t i = lenv_hash(e->capacity, k->val.strval);
    e->entries[i] = lval_copy(v);
}

void lenv_def(lenv_t *e, lval_t *k, lval_t *v) {

    while ( e->parent != NULL ) {
        e = e->parent;
    }
    lenv_put(e, k, v);
}

void lenv_pretty_print(lenv_t *e) {
    printf("DEBUG -- lenv content:\n");
    for ( size_t i = 0; i < e->capacity; ++i ) {
        if ( e->entries[i] != NULL ) {
            printf("    n: %lu t: %s p: %p\n", i, ltype_name(e->entries[i]->type), (void *) (e->entries + i));
        }
    }
    printf("DEBUG -- lenv END\n");
}

