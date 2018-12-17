#ifndef LISPER_EXEC
#define LISPER_EXEC

#include "lenv.h"
#include "grammar.h"
#include "prgparams.h"

void exec_repl(lenv_t *, struct grammar_elems);

void exec_filein(lenv_t *, struct lisper_params *);

#endif
