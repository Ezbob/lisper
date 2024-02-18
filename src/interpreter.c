#include "interpreter.h"
#include "builtin.h"
#include "value.h"
#include "value/lvalue.h"
#include "value/transformers.h"
#include "environment.h"

int linterpreter_init(struct linterpreter *intp, int argc, char **argv) {
  intp->halt_type = LINTERP_NO_HALT;
  if (argv && argc >= 0) {
    intp->argc = argc;
    intp->argv = argv;
  }
  if (lenvironment_init(&intp->lvalue_mp, &intp->env, 512) == -1) {
    return -1;
  }
  if (mempool_init(&intp->lvalue_mp, sizeof(struct lvalue), 10000) == -1) {
    return -1;
  }
  register_builtins(intp);
  grammar_elems_init(&intp->grammar);
  grammar_make_lang(&intp->grammar);
  return 0;
}

void linterpreter_destroy(struct linterpreter *intp) {
  grammar_elems_destroy(&intp->grammar);
  lenvironment_deinit(&intp->lvalue_mp, &intp->env);
  mempool_deinit(&intp->lvalue_mp);
  if (intp->halt_type == LINTERP_BAD_SYNTAX) {
    mpc_err_delete(intp->halt_value.error);
  }
}

int linterpreter_exec(struct linterpreter *intp, const char *exec, struct lvalue **out) {
  int rc = 0;
  mpc_result_t r;
  if (mpc_parse("<stdin>", exec, intp->grammar.Lisper, &r)) {
    struct lvalue *read = lvalue_read(&intp->lvalue_mp, r.output);
    struct lvalue *result = lvalue_eval(intp, read);
    if (result->type == LVAL_ERR) {
      rc = 1;
    }
    if (out) {
      *out = result;
    } else {
      lvalue_del(&intp->lvalue_mp, result);
    }
    mpc_ast_delete(r.output);
  } else {
    intp->halt_type = LINTERP_BAD_SYNTAX;
    intp->halt_value.error = r.error;
    if (out) {
      *out = NULL;
    }
    rc = 1;
  }
  return rc;
}
