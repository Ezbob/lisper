#include "lisper.h"
#include "builtin.h"
#include "environment.h"
#include "execute.h"
#include "grammar.h"
#include "mempool.h"
#include "prgparams.h"
#include "value.h"
#include "value/lvalue.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "interpreter.h"

struct argument_capture *args;

/*
void signal_handler(int signum) {
  if (signum == SIGINT) {
    exit(0);
  } else {
    grammar_elems_destroy(&elems);
    lenvironment_del(env);
  }
}

void exit_handler(void) {
  grammar_elems_destroy(&elems);
  lenvironment_del(env);
  mempool_del(lvalue_mp);
}
*/

int main(int argc, char **argv) {

  struct argument_capture capture;
  struct lisper_params params;

  struct linterpreter interpreter;

  if (linterpreter_init(&interpreter) == -1) {
    return 1;
  }

  capture.argc = argc;
  capture.argv = argv;

  args = &capture;

/*
  lvalue_mp = mempool_new(sizeof(struct lvalue), lvalue_mempool_size);

  env = lenvironment_new(hash_size);

  register_builtins(env);

  grammar_elems_init(&elems);
  grammar_make_lang(&elems);

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
