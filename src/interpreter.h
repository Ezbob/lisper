
#ifndef _HEADER_FILE_interpreter_20240215215413_
#define _HEADER_FILE_interpreter_20240215215413_

#include "environment.h"
#include "grammar.h"
#include "mempool.h"
#include <stdint.h>

struct lvalue;

struct linterpreter {
  struct lenvironment env;
  struct grammar_elems grammar;
  struct mempool lvalue_mp;
  int argc;
  char **argv;
  enum {
    LINTERP_NO_HALT,
    LINTERP_BAD_SYNTAX,
    LINTERP_USER_EXIT,
  } halt_type;
  union {
    mpc_err_t *error;
    int rc;
  } halt_value;
};

int linterpreter_init(struct linterpreter *intp, int argc, char **argv);
void linterpreter_destroy(struct linterpreter *intp);

int linterpreter_exec(struct linterpreter *intp, const char *exec, struct lvalue **out);

#endif