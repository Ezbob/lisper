#ifndef LISPER_GRAMMAR
#define LISPER_GRAMMAR

#include "mpc.h"

struct grammar_elems {
  mpc_parser_t *Boolean;
  mpc_parser_t *Integer;
  mpc_parser_t *Float;
  mpc_parser_t *String;
  mpc_parser_t *Symbol;
  mpc_parser_t *Comment;
  mpc_parser_t *Qexpr;
  mpc_parser_t *Sexpr;
  mpc_parser_t *Expr;
  mpc_parser_t *Lisper;
};

void grammar_elems_init(struct grammar_elems *);
void grammar_elems_destroy(struct grammar_elems *);
void grammar_make_lang(struct grammar_elems *);

#endif
