#include "interpreter.h"
#include "lisper.h"
#include "builtin.h"
#include "value/lvalue.h"
#include "value/constructors.h"
#include "value/transformers.h"
#include "environment.h"
#include "mpc.h"
#include "grammar.h"

int lisper_init(struct linterpreter **intpp, int argc, char **argv) {
  *intpp = malloc(sizeof(struct linterpreter));
  if (*intpp == NULL) {
    return -1;
  }
  struct linterpreter *intp = *intpp;

  if (argv && argc >= 0) {
    intp->argc = argc;
    intp->argv = argv;
  }

  intp->env = lenvironment_new(intp->lvalue_mp, 512);
  if (!intp->env) {
    free(intp);
    return -1;
  }
  intp->lvalue_mp = mempool_new(sizeof(struct lvalue), 10000);
  if (!intp->lvalue_mp) {
    free(intp);
    return -1;
  }

  intp->grammar = grammar_elems_new();
  if (!intp->grammar) {
    return -1;
  }

  grammar_elems_init(intp->grammar);
  grammar_make_lang(intp->grammar);

  register_builtins(intp);
  return 0;
}

void lisper_destroy(struct linterpreter *intp) {
  grammar_elems_destroy(intp->grammar);
  lenvironment_del(intp->lvalue_mp, intp->env);
  mempool_del(intp->lvalue_mp);
  free(intp);
}

struct lvalue *lisper_exec(struct linterpreter *intp, const char *exec) {
  mpc_result_t r;
  if (mpc_parse("<stdin>", exec, intp->grammar->Lisper, &r)) {
    struct lvalue *read = lvalue_read(intp->lvalue_mp, r.output);
    struct lvalue *result = lvalue_eval(intp, read);
    mpc_ast_delete(r.output);
    return result;
  }

  struct lvalue *err = lvalue_err(intp->lvalue_mp, "Syntax error: (%i:%i) %s", r.error->state.row, r.error->state.col, mpc_err_string(r.error));
  mpc_err_delete(r.error);
  return err;
}

int lvalue_is_int(struct lvalue *v) {
  return v->type == LVAL_INT;
}

linteger lvalue_get_int(struct lvalue *v) {
  return v->val.intval;
}

int lvalue_is_float(struct lvalue *v) {
  return v->type == LVAL_FLOAT;
}

double lvalue_get_float(struct lvalue *v) {
  return v->val.floatval;
}

int lvalue_is_string(struct lvalue *v) {
  return v->type == LVAL_STR;
}

const char *lvalue_get_string(struct lvalue *v) {
  return v->val.strval;
}

int lvalue_is_symbol(struct lvalue *v) {
  return v->type == LVAL_SYM;
}

const char *lvalue_get_symbol(struct lvalue *v) {
  return v->val.strval;
}

int lvalue_is_list(struct lvalue *v) {
  return v->type == LVAL_QEXPR;
}

llist *lvalue_get_list(struct lvalue *v) {
  return (llist *)(&v->val.list);
}

int lvalue_is_error(struct lvalue *v) {
  return v->type == LVAL_ERR;
}

const char *lvalue_get_error(struct lvalue *v) {
  return v->val.strval;
}

void lvalue_delete(struct linterpreter *intp, struct lvalue *v) {
  lvalue_del(intp->lvalue_mp, v);
}