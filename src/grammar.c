#include "mpc.h"
#include "grammar.h"


void grammar_elems_init(grammar_elems *elems) {
    elems->Integer = mpc_new("integer");
    elems->Float = mpc_new("float");
    elems->Symbol = mpc_new("symbol");
    elems->Sexpr = mpc_new("sexpr");
    elems->Qexpr = mpc_new("qexpr");
    elems->Expr = mpc_new("expr");
    elems->Lisper = mpc_new("lisper");
}

void grammar_elems_destroy(grammar_elems *elems) {
    mpc_cleanup(6,
        elems->Integer,
        elems->Float,
        elems->Symbol,
        elems->Qexpr,
        elems->Sexpr,
        elems->Expr,
        elems->Lisper
    );
}

void grammar_make_lang(grammar_elems *elems) {
    
    mpca_lang(MPCA_LANG_DEFAULT,
        "float      : /([-+])?[0-9]+\\.[0-9]+/ ;" // | /([-+])?[0-9]+(\\.[0-9]*)?[eE]([-+])?[0-9]+/ ;"
        "integer    : /([-+])?[0-9]+/ ;"
        "symbol     : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&%^]+/ ;"
        "qexpr      : '{' <expr>* '}' ;"
        "sexpr      : '(' <expr>* ')' ;"
        "expr       : <integer> | <float> | <symbol> | <sexpr> | <qexpr> ;"
        "lisper     : /^/ <expr>* /$/ ;",
        elems->Integer,
        elems->Float,
        elems->Symbol,
        elems->Qexpr,
        elems->Sexpr,
        elems->Expr, 
        elems->Lisper
    );
}

