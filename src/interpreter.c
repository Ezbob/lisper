#include "interpreter.h"
#include "builtin.h"
#include "value.h"
#include "value/lvalue.h"

void linterpreter_init(struct linterpreter *intp) {
  lenvironment_init(&intp->env, 512);
  mempool_init(&intp->lvalue_mp, sizeof(struct lvalue), 10000);
  register_builtins(&intp->env);
  grammar_elems_init(&intp->grammar);
  grammar_make_lang(&intp->grammar);
  intp->error = NULL;
}

void linterpreter_destroy(struct linterpreter *intp) {
  grammar_elems_destroy(&intp->grammar);
  lenvironment_deinit(&intp->env);
  mempool_deinit(&intp->lvalue_mp);
  if (intp->error != NULL) {
    mpc_err_delete(intp->error);
  }
}

int linterpreter_exec(struct linterpreter *intp, const char *exec, struct lvalue **out) {
  int rc = 0;
  mpc_result_t r;
  if (mpc_parse("<stdin>", exec, &intp->grammar.Lisper, &r)) {
    struct lvalue *read = lvalue_read(r.output);
    struct lvalue *result = lvalue_eval(&intp->env, read);
    if (result->type == LVAL_ERR) {
      rc = 1;
    }
    if (out) {
      *out = result;
    } else {
      lvalue_del(result);
    }
    mpc_ast_delete(r.output);
  } else {
    intp->error = r.error;
    if (out)
      *out = NULL;
    rc = 1;
  }
  return rc;
}
