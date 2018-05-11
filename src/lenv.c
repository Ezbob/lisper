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

lenv_entry_t *lenv_entry_new(void) {
    lenv_entry_t *entry = malloc(sizeof(lenv_entry_t));
    entry->name = NULL;
    entry->envval = NULL;
    entry->next = NULL;

    return entry;
}

void lenv_entry_del(lenv_entry_t *e) {
    if ( e->envval != NULL ) {
        lval_del(e->envval);
    }
    if ( e->name != NULL ) {
        free(e->name);
    }
    if ( e->next != NULL ) {
        lenv_entry_del(e->next);
    }
    free(e);
}

lenv_entry_t *lenv_entry_copy(lenv_entry_t *e) {
    lenv_entry_t *x = lenv_entry_new();

    x->name = calloc(strlen(e->name) + 1, sizeof(char));
    strcpy(x->name, e->name);
    x->envval = lval_copy(e->envval);

    if ( e->next != NULL ) {
        x->next = lenv_entry_copy(e->next);
    }
    return x;
}

lenv_t *lenv_new(size_t capacity) {
    lenv_t *env = malloc(sizeof(lenv_t));
    env->parent = NULL;
    env->entries = malloc(capacity * sizeof(lenv_entry_t *));
    env->capacity = capacity;
    for (size_t i = 0; i < capacity; ++i) {
        env->entries[i] = lenv_entry_new();
    }
    return env;
}

void lenv_del(lenv_t *env) {
    env->parent = NULL;
    for ( size_t i = 0; i < env->capacity; ++i ) {
        lenv_entry_del(env->entries[i]);
    }
    free(env->entries);
    free(env);
}

lenv_t *lenv_copy(lenv_t *env) {
    lenv_t *new = lenv_new(env->capacity);
    new->parent = env->parent;
    for (size_t i = 0; i < env->capacity; ++i ) {
        new->entries[i] = lenv_entry_copy(env->entries[i]);
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
    lenv_entry_t *entry = e->entries[i];

    if ( entry->name != NULL && entry->envval != NULL ) {
        if ( strcmp(entry->name, k->val.strval) == 0 ) {
            /* found */
            return lval_copy(entry->envval);
        }
        lenv_entry_t *iter = entry->next;
        /* go through the linked list */
        while ( iter != NULL ) {
            if ( strcmp(iter->name, k->val.strval) == 0 ) {
                return lval_copy(iter->envval);
            }
        }
    } else if ( e->parent != NULL ) {
        return lenv_get(e->parent, k);
    }

    return lval_err("Unbound symbol '%s'", k->val.strval);
}

void lenv_put(lenv_t *e, lval_t *k, lval_t *v) {
    size_t i = lenv_hash(e->capacity, k->val.strval);
    if ( e->entries[i]->next == NULL ) { /* head */
        e->entries[i]->envval = lval_copy(v);
        e->entries[i]->name = calloc(strlen(k->val.strval) + 1, sizeof(char));
        strcpy(e->entries[i]->name, k->val.strval);
    } else {
        lenv_entry_t *old = e->entries[i];
        lenv_entry_t *new = lenv_entry_new();
        new->envval = lval_copy(v);
        new->name = calloc(strlen(k->val.strval) + 1, sizeof(char));
        strcpy(new->name, k->val.strval);
        new->next = old;
        e->entries[i] = new;
    }
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
        if ( e->entries[i]->envval != NULL && e->entries[i]->name != NULL ) {
            printf("    n: %s t: %s p: %p\n", e->entries[i]->name, ltype_name(e->entries[i]->envval->type), (void *) (e->entries[i]->envval));
        }
    }
    printf("DEBUG -- lenv END\n");
}

