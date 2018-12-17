#ifndef LISPER_EXEC
#define LISPER_EXEC

#include "lenv.h"
#include "grammar.h"

void exec_repl(lenv_t *, struct grammar_elems);

void exec_filein(lenv_t *, int, char**);

#endif
