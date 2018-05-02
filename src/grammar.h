#include "mpc.h"

typedef struct grammar_elems {
    mpc_parser_t* Boolean;
    mpc_parser_t* Integer;
    mpc_parser_t* Float;
    mpc_parser_t* Symbol;
    mpc_parser_t* Qexpr;
    mpc_parser_t* Sexpr;
    mpc_parser_t* Expr;
    mpc_parser_t* Lisper;
} grammar_elems;

void grammar_elems_init(grammar_elems *);
void grammar_elems_destroy(grammar_elems *);
void grammar_make_lang(grammar_elems *);


