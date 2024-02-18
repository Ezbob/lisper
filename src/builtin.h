#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

#include "environment.h"
#include "interpreter.h"
#include "value.h"

struct lvalue *builtin_load(struct linterpreter *intp, struct lvalue *v);

void register_builtins(struct linterpreter *inpt);

#endif
