#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lval.h"
#include "lenv.h"

struct lvalue *builtin_load(struct lenvironment *, struct lvalue *);

void register_builtins(struct lenvironment *e);

#endif

