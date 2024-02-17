#include "grammar.h"
#include "mpc.h"


void grammar_elems_init(struct grammar_elems *elems) {
  elems->Boolean = mpc_new("boolean");
  elems->Integer = mpc_new("integer");
  elems->Float = mpc_new("float");
  elems->String = mpc_new("string");
  elems->Comment = mpc_new("comment");
  elems->Symbol = mpc_new("symbol");
  elems->Sexpr = mpc_new("sexpr");
  elems->Qexpr = mpc_new("qexpr");
  elems->Expr = mpc_new("expr");
  elems->Lisper = mpc_new("lisper");
}

void grammar_elems_destroy(struct grammar_elems *elems) {
  mpc_cleanup(10, elems->Boolean, elems->Integer, elems->Float, elems->Comment,
              elems->String, elems->Symbol, elems->Qexpr, elems->Sexpr, elems->Expr,
              elems->Lisper);
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
