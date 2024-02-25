#ifndef LISPER_BUILTIN
#define LISPER_BUILTIN

struct linterpreter;
struct lvalue;

struct lvalue *builtin_load(struct linterpreter *intp, struct lvalue *v);

void register_builtins(struct linterpreter *inpt);

#endif
