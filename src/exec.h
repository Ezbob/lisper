#ifndef LISPER_EXEC
#define LISPER_EXEC

#include "lenv.h"
#include "grammar.h"
#include "prgparams.h"

void exec_repl(struct lenv_t *, struct grammar_elems);

void exec_filein(struct lenv_t *, struct lisper_params *);

#endif
