#include "mpc.h"

typedef struct grammar_elems {
    mpc_parser_t* Number;
    mpc_parser_t* UnOp;
    mpc_parser_t* BinOp;
    mpc_parser_t* Expr;
    mpc_parser_t* Lisper;
} grammar_elems;

void grammar_elems_init(grammar_elems *);
void grammar_elems_destroy(grammar_elems *);
void grammar_make_lang(grammar_elems *);


