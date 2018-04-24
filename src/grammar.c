#include "mpc.h"
#include "grammar.h"


void grammar_elems_init(grammar_elems *elems) {
    elems->Number = mpc_new("number");
    elems->Symbol = mpc_new("symbol");
    elems->Sexpr = mpc_new("sexpr");
    elems->Qexpr = mpc_new("qexpr");
    elems->Expr = mpc_new("expr");
    elems->Lisper = mpc_new("lisper");
}

void grammar_elems_destroy(grammar_elems *elems) {
    mpc_cleanup(6,
        elems->Number,
        elems->Symbol,
        elems->Qexpr,
        elems->Sexpr,
        elems->Expr,
        elems->Lisper
    );
}

void grammar_make_lang(grammar_elems *elems) {
    
    mpca_lang(MPCA_LANG_DEFAULT,
        "number     : /(-)?[0-9]+(\\.[0-9]*)?/ ;"
        "symbol     : '+' | '-' | '*' | '/' | '^' | '%' | \"min\" | \"max\""
            " | \"list\" | \"head\" | \"tail\" | \"join\" | \"eval\" | \"cons\" | \"len\" ;"
        "qexpr      : '{' <expr>* '}' ;"
        "sexpr      : '(' <expr>* ')' ;"
        "expr       : <number> | <symbol> | <sexpr> | <qexpr> ;"
        "lisper     : /^/ <expr>* /$/ ;",
        elems->Number, 
        elems->Symbol,
        elems->Qexpr,
        elems->Sexpr,
        elems->Expr, 
        elems->Lisper
    );
}

