#include "grammar.h"
#include "mpc.h"

struct grammar_elems *grammar_elems_new() {
  struct grammar_elems *result = malloc(sizeof(struct grammar_elems));
  if (!result) {
    return NULL;
  }
  int rc = grammar_elems_init(result);
  if (rc == -1) {
    free(result);
    return NULL;
  }
  return result;
}

int grammar_elems_init(struct grammar_elems *elems) {
  elems->Boolean = mpc_new("boolean");
  if (!elems->Boolean) {
    return -1;
  }
  elems->Integer = mpc_new("integer");
  if (!elems->Integer) {
    mpc_delete(elems->Boolean);
    return -1;
  }
  elems->Float = mpc_new("float");
  if (!elems->Float) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    return -1;
  }
  elems->String = mpc_new("string");
  if (!elems->String) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    mpc_delete(elems->Float);
    return -1;
  }
  elems->Comment = mpc_new("comment");
  if (!elems->Comment) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    mpc_delete(elems->Float);
    mpc_delete(elems->String);
    return -1;
  }
  elems->Symbol = mpc_new("symbol");
  if (!elems->Symbol) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    mpc_delete(elems->Float);
    mpc_delete(elems->String);
    mpc_delete(elems->Comment);
    return -1;
  }
  elems->Sexpr = mpc_new("sexpr");
  if (!elems->Sexpr) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    mpc_delete(elems->Float);
    mpc_delete(elems->String);
    mpc_delete(elems->Comment);
    mpc_delete(elems->Symbol);
    return -1;
  }
  elems->Qexpr = mpc_new("qexpr");
  if (!elems->Qexpr) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    mpc_delete(elems->Float);
    mpc_delete(elems->String);
    mpc_delete(elems->Comment);
    mpc_delete(elems->Symbol);
    mpc_delete(elems->Sexpr);
    return -1;
  }
  elems->Expr = mpc_new("expr");
  if (!elems->Expr) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    mpc_delete(elems->Float);
    mpc_delete(elems->String);
    mpc_delete(elems->Comment);
    mpc_delete(elems->Symbol);
    mpc_delete(elems->Sexpr);
    mpc_delete(elems->Qexpr);
    return -1;
  }
  elems->Lisper = mpc_new("lisper");
  if (!elems->Lisper) {
    mpc_delete(elems->Boolean);
    mpc_delete(elems->Integer);
    mpc_delete(elems->Float);
    mpc_delete(elems->String);
    mpc_delete(elems->Comment);
    mpc_delete(elems->Symbol);
    mpc_delete(elems->Sexpr);
    mpc_delete(elems->Qexpr);
    mpc_delete(elems->Expr);
    return -1;
  }
  return 0;
}

void grammar_elems_destroy(struct grammar_elems *elems) {
    mpc_cleanup(10, elems->Boolean, elems->Integer, elems->Float, elems->Comment,
              elems->String, elems->Symbol, elems->Qexpr, elems->Sexpr, elems->Expr,
              elems->Lisper);
    free(elems);
}

void grammar_make_lang(struct grammar_elems *elems) {
  mpca_lang(
      MPCA_LANG_DEFAULT,
      "string     : /\"(\\\\.|[^\"])*\"/ ;"
      "boolean    : \"true\" | \"false\" ;"
      "float      : /([-+])?[0-9]+(\\.[0-9]*)?[eE][0-9]+/ | /([-+])?[0-9]+\\.[0-9]*/ ;"
      "integer    : /([-+])?[0-9]+/ ;"
      "symbol     : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%|.]+/ ;"
      "comment    : /;[^\\r\\n]*/ ;"
      "qexpr      : '{' <expr>* '}' ;"
      "sexpr      : '(' <expr>* ')' ;"
      "expr       : <string> | <boolean> | <float> | <integer> | <symbol>"
      "| <sexpr>  | <qexpr>   | <comment> ;"
      "lisper     : /^/ <expr>* /$/ ;",
      elems->Boolean, elems->Integer, elems->Float, elems->String, elems->Symbol,
      elems->Comment, elems->Qexpr, elems->Sexpr, elems->Expr, elems->Lisper);
}
