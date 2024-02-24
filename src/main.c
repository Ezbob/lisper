#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include "lisper.h"
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

  //struct lisper_params params;

  struct linterpreter *interpreter;

  if (lisper_init(&interpreter, argc, argv) == -1) {
    return 1;
  }

  struct lvalue *out;
  out = lisper_exec(interpreter, "(+ 2 (+ 1 3) (* 4 2))");
  if (!lvalue_is_error(out)) {

    if (lvalue_is_int(out)) {
      lvalue_get_int(out);
      printf("result -> %lli\n", lvalue_get_int(out));
    }

  } else {
    fprintf(stderr, "error: %s\n", lvalue_get_error(out));
  }

  lvalue_delete(interpreter, out);
/*


  signal(SIGINT, signal_handler);

  atexit(exit_handler);

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
*/
  lisper_destroy(interpreter);
  return 0;
}
