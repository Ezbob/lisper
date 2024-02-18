
#ifndef _HEADER_FILE_interpreter_20240215215413_
#define _HEADER_FILE_interpreter_20240215215413_

#include "environment.h"
#include "grammar.h"
#include "mempool.h"
#include "value.h"

struct linterpreter {
  struct lenvironment env;
  struct grammar_elems grammar;
  struct mempool lvalue_mp;
  mpc_err_t *error;
};

int linterpreter_init(struct linterpreter *intp);
void linterpreter_destroy(struct linterpreter *dest);

int linterpreter_exec(struct linterpreter *intp, const char *exec, struct lvalue **out);

#endif