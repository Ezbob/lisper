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

struct lenv_entry_t *lenv_entry_new(void) {
    struct lenv_entry_t *entry = malloc(sizeof(struct lenv_entry_t));
    entry->name = NULL;
    entry->envval = NULL;
    entry->next = NULL;

    return entry;
}

void lenv_entry_del(struct lenv_entry_t *e) {
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

struct lenv_entry_t *lenv_entry_copy(struct lenv_entry_t *e) {
    struct lenv_entry_t *x = lenv_entry_new();

    if ( !(e->name == NULL || e->envval == NULL) ) {
        x->name = calloc(strlen(e->name) + 1, sizeof(char));
        strcpy(x->name, e->name);
        x->envval = lval_copy(e->envval);
    }

    if ( e->next != NULL ) {
        x->next = lenv_entry_copy(e->next);
    }
    return x;
}

struct lenv_t *lenv_new(size_t capacity) {
    struct lenv_t *env = malloc(sizeof(struct lenv_t));
    env->parent = NULL;
    env->entries = malloc(capacity * sizeof(struct lenv_entry_t *));
    env->capacity = capacity;
    for (size_t i = 0; i < capacity; ++i) {
        env->entries[i] = NULL;
    }
    return env;
}

void lenv_del(struct lenv_t *env) {
    if ( env == NULL ) {
        return;
    }
    env->parent = NULL;
    for ( size_t i = 0; i < env->capacity; ++i ) {
        if ( env->entries[i] != NULL ) {
            lenv_entry_del(env->entries[i]);
        }
    }
    free(env->entries);
    free(env);
}

struct lenv_t *lenv_copy(struct lenv_t *env) {
    struct lenv_t *new = lenv_new(env->capacity);
    new->parent = env->parent;
    for ( size_t i = 0; i < env->capacity; ++i ) {
        if ( env->entries[i] != NULL ) {
            new->entries[i] = lenv_entry_copy(env->entries[i]);
        }
    }

    return new;
}

void lenv_add_builtin(struct lenv_t *e, char *name, lbuiltin func) {
    struct lval_t *k = lval_sym(name);
    struct lval_t *v = lval_builtin(func);

    lenv_put(e, k, v);

    lval_del(k);
    lval_del(v);
}

struct lval_t *lenv_get(struct lenv_t *e, struct lval_t *k) {

    size_t i = lenv_hash(e->capacity, k->val.strval);
    struct lenv_entry_t *entry = e->entries[i];

    if ( entry != NULL ) {
        if ( strcmp(entry->name, k->val.strval) == 0 ) {
            /* found */
            return lval_copy(entry->envval);
        }
        struct lenv_entry_t *iter = entry->next;
        /* go through the linked list */
        while ( iter != NULL ) {
            if ( strcmp(iter->name, k->val.strval) == 0 ) {
                /* found in chain */
                return lval_copy(iter->envval);
            }
        }
    } else if ( e->parent != NULL ) {
        return lenv_get(e->parent, k);
    }

    return lval_err("Unbound symbol '%s'", k->val.strval);
}

void lenv_put(struct lenv_t *e, struct lval_t *k, struct lval_t *v) {
    size_t i = lenv_hash(e->capacity, k->val.strval);

    struct lenv_entry_t *entry = e->entries[i];
    if ( entry == NULL ) {
        /* chain is empty */
        entry = lenv_entry_new();
        entry->envval = lval_copy(v);
        entry->name = calloc(strlen(k->val.strval) + 1, sizeof(char));
        strcpy(entry->name, k->val.strval);
        e->entries[i] = entry;
    } else {
        /* chain is non-empty */
        struct lenv_entry_t *iter = entry;

        while ( iter != NULL ) {
            if ( strcmp(iter->name, k->val.strval) == 0 ) {
                /* match in the chain --> override sematics */
                lval_del(iter->envval);
                iter->envval = lval_copy(v);
                return;
            }
            iter = iter->next;
        }

        /* not found in chain --> offer to front */

        struct lenv_entry_t *old = entry;
        struct lenv_entry_t *new = lenv_entry_new();
        new->envval = lval_copy(v);
        new->name = calloc(strlen(k->val.strval) + 1, sizeof(char));
        strcpy(new->name, k->val.strval);
        new->next = old;
        e->entries[i] = new;
    }
}

void lenv_def(struct lenv_t *e, struct lval_t *k, struct lval_t *v) {

    while ( e->parent != NULL ) {
        e = e->parent;
    }
    lenv_put(e, k, v);
}

void lenv_pretty_print(struct lenv_t *e) {
    for ( size_t i = 0; i < e->capacity; ++i ) {
        if ( e->entries[i] != NULL ) {
            printf("i: %lu    (n: '%s' t: '%s' p: %p)", i, e->entries[i]->name, ltype_name(e->entries[i]->envval->type), (void *) (e->entries[i]->envval));
            struct lenv_entry_t *iter = e->entries[i]->next;
            while ( iter != NULL ) {
                printf("-o-(n: '%s' t: '%s' p: %p)", iter->name, ltype_name(iter->envval->type), (void *) iter->envval);
                iter = iter->next;
            }
            printf("\n");
        }
    }
}

