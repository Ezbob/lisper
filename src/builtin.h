#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "lvalue.h"
#include "lenvironment.h"

struct lvalue *builtin_load(struct lenvironment *, struct lvalue *);

void register_builtins(struct lenvironment *e);

#endif

