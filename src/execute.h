#ifndef LISPER_EXEC
#define LISPER_EXEC

#include "environment.h"
#include "grammar.h"
#include "prgparams.h"

int exec_repl(struct lenvironment *, struct grammar_elems *);

int exec_filein(struct lenvironment *, struct lisper_params *);

int exec_eval(struct lenvironment *env, struct grammar_elems *elems,
              struct lisper_params *params);

#endif
