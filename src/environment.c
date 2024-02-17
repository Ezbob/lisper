#include "environment.h"
#include <stdlib.h>
#include <string.h>
#include "builtin.h"
#include "value.h"
#include "compat_string.h"
#include "value/lvalue.h"
#include "value/constructors.h"

size_t lenvironment_hash(size_t capacity, char *key) {
    size_t i;
    size_t res = 0;
    size_t key_size = strlen(key);

    for ( i = 0; i < key_size; ++i ) {
        res += key[i];
        if ( i < key_size - 1 ) {
            res <<= 1;
        }
    }

    return res % capacity;
}

struct lenvironment_entry *lenvironment_entry_new() {
    struct lenvironment_entry *entry = malloc(sizeof(struct lenvironment_entry));
    entry->name = NULL;
    entry->envval = NULL;
    entry->next = NULL;

    return entry;
}

void lenvironment_entry_del(struct lenvironment_entry *e) {
    if ( e->envval != NULL ) {
        lvalue_del(e->envval);
    }
    if ( e->name != NULL ) {
        free(e->name);
    }
    if ( e->next != NULL ) {
        lenvironment_entry_del(e->next);
    }
    free(e);
}

struct lenvironment_entry *lenvironment_entry_copy(struct lenvironment_entry *e) {
    struct lenvironment_entry *x = lenvironment_entry_new();

    if ( !(e->name == NULL || e->envval == NULL) ) {
        x->name = calloc(strlen(e->name) + 1, sizeof(char));
        strcpy(x->name, e->name);
        x->envval = lvalue_copy(e->envval);
    }

    if ( e->next != NULL ) {
        x->next = lenvironment_entry_copy(e->next);
    }
    return x;
}

void lenvironment_init(struct lenvironment *env, size_t capacity) {
    env->parent = NULL;
    env->entries = malloc(capacity * sizeof(struct lenvironment_entry *));
    env->capacity = capacity;
    for (size_t i = 0; i < capacity; ++i) {
        env->entries[i] = NULL;
    }
}

void lenvironment_deinit(struct lenvironment *env) {
    env->parent = NULL;
    for ( size_t i = 0; i < env->capacity; ++i ) {
        if ( env->entries[i] != NULL ) {
            lenvironment_entry_del(env->entries[i]);
        }
    }
    free(env->entries);
}

struct lenvironment *lenvironment_new(size_t capacity) {
    struct lenvironment *env = malloc(sizeof(struct lenvironment));
    lenvironment_init(env, capacity);
    return env;
}

void lenvironment_del(struct lenvironment *env) {
    if ( env == NULL ) {
        return;
    }
    lenvironment_deinit(env);
    free(env);
}

struct lenvironment *lenvironment_copy(struct lenvironment *env) {
    struct lenvironment *new = lenvironment_new(env->capacity);
    new->parent = env->parent;
    for ( size_t i = 0; i < env->capacity; ++i ) {
        if ( env->entries[i] != NULL ) {
            new->entries[i] = lenvironment_entry_copy(env->entries[i]);
        }
    }

    return new;
}

void lenvironment_add_builtin(struct lenvironment *e, char *name, struct lvalue *( *func)(struct lenvironment *, struct lvalue *)) {
    struct lvalue *k = lvalue_sym(name);
    struct lvalue *v = lvalue_builtin(func);

    lenvironment_put(e, k, v);

    lvalue_del(k);
    lvalue_del(v);
}

struct lvalue *lenvironment_get(struct lenvironment *e, struct lvalue *k) {

    size_t i = lenvironment_hash(e->capacity, k->val.strval);
    struct lenvironment_entry *entry = e->entries[i];

    if ( entry != NULL ) {
        if ( strcmp(entry->name, k->val.strval) == 0 ) {
            return lvalue_copy(entry->envval);
        }
        struct lenvironment_entry *entry_iter = entry->next;
        /* go through the linked list */
        while ( entry_iter != NULL ) {
            if ( strcmp(entry_iter->name, k->val.strval) == 0 ) {
                /* found in chain */
                return lvalue_copy(entry_iter->envval);
            }
        }
    } else if ( e->parent != NULL ) {
        for (
            struct lenvironment *environment_iter = e->parent;
            environment_iter != NULL;
            environment_iter = environment_iter->parent
        ) {
            i = lenvironment_hash(environment_iter->capacity, k->val.strval);
            entry = environment_iter->entries[i];
            if ( entry != NULL ) {
                break;
            }
        }

        if (entry != NULL) {
            if ( strcmp(entry->name, k->val.strval) == 0 ) {
                return lvalue_copy(entry->envval);
            }
            struct lenvironment_entry *entry_iter = entry->next;
            /* go through the linked list */
            while ( entry_iter != NULL ) {
                if ( strcmp(entry_iter->name, k->val.strval) == 0 ) {
                    /* found in chain */
                    return lvalue_copy(entry_iter->envval);
                }
            }
        }
    }

    return lvalue_err("Unbound symbol '%s'", k->val.strval);
}

void lenvironment_put(struct lenvironment *e, struct lvalue *k, struct lvalue *v) {
    size_t i = lenvironment_hash(e->capacity, k->val.strval);

    struct lenvironment_entry *entry = e->entries[i];
    if ( entry == NULL ) {
        /* chain is empty */
        entry = lenvironment_entry_new();
        entry->envval = lvalue_copy(v);
        entry->name = strdup(k->val.strval);
        e->entries[i] = entry;
    } else {
        /* chain is non-empty */
        struct lenvironment_entry *iter = entry;

        while ( iter != NULL ) {
            if ( strcmp(iter->name, k->val.strval) == 0 ) {
                /* match in the chain --> override sematics */
                lvalue_del(iter->envval);
                iter->envval = lvalue_copy(v);
                return;
            }
            iter = iter->next;
        }

        /* not found in chain --> offer to front */

        struct lenvironment_entry *old = entry;
        struct lenvironment_entry *new = lenvironment_entry_new();
        new->envval = lvalue_copy(v);
        new->name = strdup(k->val.strval);
        new->next = old;
        e->entries[i] = new;
    }
}

void lenvironment_def(struct lenvironment *e, struct lvalue *k, struct lvalue *v) {

    while ( e->parent != NULL ) {
        e = e->parent;
    }
    lenvironment_put(e, k, v);
}

void lenvironment_pretty_print(struct lenvironment *e) {
    for ( size_t i = 0; i < e->capacity; ++i ) {
        if ( e->entries[i] != NULL ) {
            printf("i: %zu    (n: '%s' t: '%s' p: %p)", i, e->entries[i]->name, ltype_name(e->entries[i]->envval->type), (void *) (e->entries[i]->envval));
            struct lenvironment_entry *iter = e->entries[i]->next;
            while ( iter != NULL ) {
                printf("-o-(n: '%s' t: '%s' p: %p)", iter->name, ltype_name(iter->envval->type), (void *) iter->envval);
                iter = iter->next;
            }
            printf("\n");
        }
    }
}

