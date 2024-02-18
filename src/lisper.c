#include "lisper.h"
#include "builtin.h"
#include "environment.h"
#include "execute.h"
#include "grammar.h"
#include "mempool.h"
#include "prgparams.h"
#include "value/lvalue.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "interpreter.h"

/*
void signal_handler(int signum) {
  if (signum == SIGINT) {
    exit(0);
  } else {
    grammar_elems_destroy(&elems);
    lenvironment_del(env);
  }
}

*/

int main(int argc, char **argv) {

  struct lisper_params params;

  struct linterpreter interpreter;

  if (linterpreter_init(&interpreter, argc, argv) == -1) {
    return 1;
  }

/*


  signal(SIGINT, signal_handler);

  atexit(exit_handler);
*/
  if (parse_prg_params(argc, argv, &params) != 0) {
    fprintf(stderr, "Error: Couldn't parse lisper program arguments\n");
    exit_with_help(1);
  }

  if (handle_prg_params(&params) != 0) {
    fprintf(stderr, "Error: Encountered error in handling lisper arguments\n");
    exit_with_help(1);
  }


  int rc = 0;
  if (params.filename != NULL) {
    rc = exec_filein(&interpreter, &params);
  } else if (params.command != NULL) {
    rc = exec_eval(&interpreter, &params);
  } else {
    rc = exec_repl(&interpreter);
  }

  linterpreter_destroy(&interpreter);
  return rc;
}
