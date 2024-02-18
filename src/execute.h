#ifndef LISPER_EXEC
#define LISPER_EXEC

#include "environment.h"
#include "mempool.h"
#include "grammar.h"
#include "prgparams.h"

struct linterpreter;

int exec_repl(struct linterpreter *);

int exec_filein(struct linterpreter *, struct lisper_params *);

int exec_eval(struct linterpreter *,
              struct lisper_params *params);

#endif
