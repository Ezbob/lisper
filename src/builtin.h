#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "value.h"
#include "environment.h"

struct lvalue *builtin_load(struct lenvironment *, struct lvalue *);

void register_builtins(struct lenvironment *e);

#endif

