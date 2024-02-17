#include "execute.h"
#include "builtin.h"
#include "environment.h"
#include "grammar.h"
#include "lisper.h"
#include "value.h"
#include "value/lvalue.h"
#include <stdio.h>
#include <stdlib.h>

#ifdef _WIN32
#include <string.h>
/* windows support */
static char buf[2048];

char *linenoise(char *prompt) {
  fputs(prompt, stdout);
  fgets(buf, 2048, stdin);
  char *cpy = malloc(strlen(buf) + 1);
  strcpy(cpy, buf);

  cpy[strlen(cpy) - 1] = '\0';
  return cpy;
}

void linenoiseHistoryAdd(char *unused) {}

#else
#include "linenoise.h"
#endif

void goodbye_exit(void) {
  printf("Bye.");
  putchar('\n');
}

int exec_repl(struct lenvironment *env, struct grammar_elems *elems) {
  char *input = NULL;
  int rc = 0;
  printf("lisper version %s\n", LISPER_VERSION);
  printf("Anders Busch 2018\n");
  puts("Press Ctrl+c to Exit\n");

  mpc_result_t r;
  struct lvalue *val;

  atexit(goodbye_exit);

#ifdef _DEBUG
  lenvironment_pretty_print(env);
#endif
  while (1) {
    input = linenoise("lisper>>> ");
    if (input == NULL) {
      break;
    } else if (strlen(input) == 0) {
      free(input);
      continue;
    }

    linenoiseHistoryAdd(input);

    if (mpc_parse("<stdin>", input, elems->Lisper, &r)) {
      struct lvalue *read = lvalue_read(r.output);
#ifdef _DEBUG
      printf("Parsed input:\n");
      mpc_ast_print(r.output);
      printf("Lval object:\n");
      lvalue_pretty_print(read);
      printf("Current env:\n");
      lenvironment_pretty_print(env);
      printf("Eval result:\n");
#endif
      val = lvalue_eval(env, read);
      lvalue_println(val);
      lvalue_del(val);
      mpc_ast_delete(r.output);
    } else {
      mpc_err_print(r.error);
      mpc_err_delete(r.error);
      rc = 1;
    }
    free(input);
  }
  return rc;
}

int exec_filein(struct lenvironment *env, struct lisper_params *params) {
  int rc = 0;
  struct lvalue *args = lvalue_add(lvalue_sexpr(), lvalue_str(params->filename));
  struct lvalue *x = builtin_load(env, args);
  if (x->type == LVAL_ERR) {
    lvalue_println(x);
    rc = 1;
  }
  lvalue_del(x);
  return rc;
}

int exec_eval(struct lenvironment *env, struct grammar_elems *elems,
              struct lisper_params *params) {
  int rc = 0;
  mpc_result_t r;
  if (mpc_parse("<stdin>", params->command, elems->Lisper, &r)) {
    struct lvalue *read = lvalue_read(r.output);
#ifdef _DEBUG
    printf("Parsed input:\n");
    mpc_ast_print(r.output);
    printf("Lval object:\n");
    lvalue_pretty_print(read);
    printf("Current env:\n");
    lenvironment_pretty_print(env);
    printf("Eval result:\n");
#endif
    struct lvalue *val = lvalue_eval(env, read);
    lvalue_println(val);
    if (val->type == LVAL_ERR) {
      rc = 1;
    }
    lvalue_del(val);
    mpc_ast_delete(r.output);
  } else {
    mpc_err_print(r.error);
    mpc_err_delete(r.error);
    rc = 1;
  }
  return rc;
}
