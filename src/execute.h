#ifndef LISPER_EXEC
#define LISPER_EXEC

#include "lenvironment.h"
#include "grammar.h"
#include "prgparams.h"

void exec_repl(struct lenvironment *, struct grammar_elems *);

void exec_filein(struct lenvironment *, struct lisper_params *);

#endif
