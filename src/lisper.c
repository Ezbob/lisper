#include "lisper_internal.h"
#include "lisper.h"
#include "builtin.h"
#include "value/lvalue.h"
#include "value/constructors.h"
#include "value/transformers.h"
#include "environment.h"
#include "mpc.h"
#include "grammar.h"

int linterpreter_init(struct linterpreter **intpp, int argc, char **argv) {
  *intpp = malloc(sizeof(struct linterpreter));
  if (*intpp == NULL) {
    return -1;
  }
  struct linterpreter *intp = *intpp;
  intp->halt_type = LINTERP_NO_HALT;
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

void linterpreter_destroy(struct linterpreter *intp) {
  grammar_elems_destroy(intp->grammar);
  lenvironment_del(intp->lvalue_mp, intp->env);
  mempool_del(intp->lvalue_mp);
  if (intp->halt_type == LINTERP_BAD_SYNTAX) {
    free(intp->halt_value.error);
  }
  free(intp);
}

int linterpreter_exec(struct linterpreter *intp, const char *exec, struct lvalue **out) {
  int rc = 0;
  mpc_result_t r;
  if (mpc_parse("<stdin>", exec, intp->grammar->Lisper, &r)) {
    struct lvalue *read = lvalue_read(intp->lvalue_mp, r.output);
    struct lvalue *result = lvalue_eval(intp, read);
    if (intp->halt_type == LINTERP_USER_EXIT) {
      lvalue_del(intp->lvalue_mp, result);
      return intp->halt_value.rc;
    }
    if (result->type == LVAL_ERR) {
      rc = -1;
    }
    if (out) {
      *out = result;
    } else {
      lvalue_del(intp->lvalue_mp, result);
    }
    mpc_ast_delete(r.output);
  } else {
    intp->halt_type = LINTERP_BAD_SYNTAX;
    intp->halt_value.error = mpc_err_string(r.error);
    mpc_err_delete(r.error);
    if (out) {
      *out = NULL;
    }
    rc = -1;
  }
  return rc;
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

void lvalue_delete(struct linterpreter *intp, struct lvalue *v) {
  lvalue_del(intp->lvalue_mp, v);
}