#include "environment.h"
#include "builtin.h"
#include "compat/string.h"
#include "value/constructors.h"
#include "value/transformers.h"
#include "value/lvalue.h"
#include "lisper_internal.h"
#include "value/print.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

size_t lenvironment_hash(size_t capacity, char *key) {
  size_t i;
  size_t res = 0;
  size_t key_size = strlen(key);

  for (i = 0; i < key_size; ++i) {
    res += key[i];
    if (i < key_size - 1) {
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

void lenvironment_entry_del(struct mempool *mp, struct lenvironment_entry *e) {
  if (e->envval != NULL) {
    lvalue_del(mp, e->envval);
  }
  if (e->name != NULL) {
    free(e->name);
  }
  if (e->next != NULL) {
    lenvironment_entry_del(mp, e->next);
  }
  free(e);
}

struct lenvironment_entry *lenvironment_entry_copy(struct mempool *mp, struct lenvironment_entry *e) {
  struct lenvironment_entry *x = lenvironment_entry_new();

  if (!(e->name == NULL || e->envval == NULL)) {
    x->name = calloc(strlen(e->name) + 1, sizeof(char));
    strcpy(x->name, e->name);
    x->envval = lvalue_copy(mp, e->envval);
  }

  if (e->next != NULL) {
    x->next = lenvironment_entry_copy(mp, e->next);
  }
  return x;
}

int lenvironment_init(struct mempool *mp, struct lenvironment *env, size_t capacity) {
  (void)(mp);
  env->parent = NULL;
  env->entries = calloc(capacity, sizeof(struct lenvironment_entry *));
  if (env->entries == NULL) {
    return -1;
  }
  env->capacity = capacity;
  return 0;
}

void lenvironment_deinit(struct mempool *mp, struct lenvironment *env) {
  env->parent = NULL;
  for (size_t i = 0; i < env->capacity; ++i) {
    if (env->entries[i] != NULL) {
      lenvironment_entry_del(mp, env->entries[i]);
    }
  }
  free(env->entries);
}

struct lenvironment *lenvironment_new(struct mempool *mp, size_t capacity) {
  struct lenvironment *env = malloc(sizeof(struct lenvironment));
  if (!env) {
    return NULL;
  }
  if ( lenvironment_init(mp, env, capacity) == -1) {
    free(env);
    return NULL;
  }
  return env;
}

void lenvironment_del(struct mempool *mp, struct lenvironment *env) {
  if (env == NULL) {
    return;
  }
  lenvironment_deinit(mp, env);
  free(env);
}

struct lenvironment *lenvironment_copy(struct mempool *mp, struct lenvironment *env) {
  struct lenvironment *new = lenvironment_new(mp, env->capacity);
  new->parent = env->parent;
  for (size_t i = 0; i < env->capacity; ++i) {
    if (env->entries[i] != NULL) {
      new->entries[i] = lenvironment_entry_copy(mp, env->entries[i]);
    }
  }

  return new;
}

void lenvironment_add_builtin(struct linterpreter *intp, char *name, builtin_func_ptr func) {
  struct lvalue *k = lvalue_sym(intp->lvalue_mp, name);
  struct lvalue *v = lvalue_builtin(intp->lvalue_mp, func);

  lenvironment_put(intp->lvalue_mp, intp->env, k, v);

  lvalue_del(intp->lvalue_mp, k);
  lvalue_del(intp->lvalue_mp, v);
}

struct lvalue *lenvironment_get(struct mempool *mp, struct lenvironment *e, struct lvalue *k) {

  size_t i = lenvironment_hash(e->capacity, k->val.strval);
  struct lenvironment_entry *entry = e->entries[i];

  if (entry != NULL) {
    if (strcmp(entry->name, k->val.strval) == 0) {
      return lvalue_copy(mp, entry->envval);
    }
    struct lenvironment_entry *entry_iter = entry->next;
    /* go through the linked list */
    while (entry_iter != NULL) {
      if (strcmp(entry_iter->name, k->val.strval) == 0) {
        /* found in chain */
        return lvalue_copy(mp, entry_iter->envval);
      }
    }
  } else if (e->parent != NULL) {
    for (struct lenvironment *environment_iter = e->parent; environment_iter != NULL;
         environment_iter = environment_iter->parent) {
      i = lenvironment_hash(environment_iter->capacity, k->val.strval);
      entry = environment_iter->entries[i];
      if (entry != NULL) {
        break;
      }
    }

    if (entry != NULL) {
      if (strcmp(entry->name, k->val.strval) == 0) {
        return lvalue_copy(mp, entry->envval);
      }
      struct lenvironment_entry *entry_iter = entry->next;
      /* go through the linked list */
      while (entry_iter != NULL) {
        if (strcmp(entry_iter->name, k->val.strval) == 0) {
          /* found in chain */
          return lvalue_copy(mp, entry_iter->envval);
        }
      }
    }
  }

  return lvalue_err(mp, "Unbound symbol '%s'", k->val.strval);
}

void lenvironment_put(struct mempool *mp, struct lenvironment *e, struct lvalue *k, struct lvalue *v) {
  size_t i = lenvironment_hash(e->capacity, k->val.strval);

  struct lenvironment_entry *entry = e->entries[i];
  if (entry == NULL) {
    /* chain is empty */
    entry = lenvironment_entry_new();
    entry->envval = lvalue_copy(mp, v);
    entry->name = strdup(k->val.strval);
    e->entries[i] = entry;
  } else {
    /* chain is non-empty */
    struct lenvironment_entry *iter = entry;

    while (iter != NULL) {
      if (strcmp(iter->name, k->val.strval) == 0) {
        /* match in the chain --> override sematics */
        lvalue_del(mp, iter->envval);
        iter->envval = lvalue_copy(mp, v);
        return;
      }
      iter = iter->next;
    }

    /* not found in chain --> offer to front */

    struct lenvironment_entry *old = entry;
    struct lenvironment_entry *new = lenvironment_entry_new();
    new->envval = lvalue_copy(mp, v);
    new->name = strdup(k->val.strval);
    new->next = old;
    e->entries[i] = new;
  }
}

void lenvironment_def(struct mempool *mp, struct lenvironment *e, struct lvalue *k, struct lvalue *v) {
  while (e->parent != NULL) {
    e = e->parent;
  }
  lenvironment_put(mp, e, k, v);
}

void lenvironment_pretty_print(struct lenvironment *e) {
  for (size_t i = 0; i < e->capacity; ++i) {
    struct lenvironment_entry *entry = e->entries[i];
    if (entry != NULL) {
      printf("i: %zu    (n: '%s' t: '%s' p: %p)", i, entry->name,
             ltype_name(entry->envval->type), (void *)(entry->envval));
      struct lenvironment_entry *iter = entry->next;
      while (iter != NULL) {
        printf("-o-(n: '%s' t: '%s' p: %p)", iter->name, ltype_name(iter->envval->type),
               (void *)iter->envval);
        iter = iter->next;
      }
      printf("\n");
    }
  }
}
