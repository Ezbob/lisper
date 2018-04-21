#include "mpc.h"
#include "grammar.h"


void grammar_elems_init(grammar_elems *elems) {
    elems->Number = mpc_new("number");
    elems->Operator = mpc_new("operator");
    elems->Expr = mpc_new("expr");
    elems->Lisper = mpc_new("lisper");
}

void grammar_elems_destroy(grammar_elems *elems) {
    mpc_cleanup(4, elems->Number, elems->Operator, elems->Expr, elems->Lisper);
}

void grammar_make_lang(grammar_elems *elems) {
    
    mpca_lang(MPCA_LANG_DEFAULT,
        "number     : /(-)?[0-9]+(\\.[0-9]*)?/ ;"
        "operator   : '+' | '-' | '*' | '/' | '^' ;"
        "expr       : <number> |  '(' <operator> <expr>+ ')' ;"
        "lisper     : /^/ <operator> <expr>+ /$/ ;",
        elems->Number, 
        elems->Operator,
        elems->Expr, 
        elems->Lisper
    );
}

