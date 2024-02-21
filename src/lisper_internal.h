
#ifndef _HEADER_FILE_lisper_internal_20240220202057_
#define _HEADER_FILE_lisper_internal_20240220202057_

struct lenvironment;
struct mempool;
struct grammar_elems;

struct linterpreter {
  struct lenvironment *env;
  struct grammar_elems *grammar;
  struct mempool *lvalue_mp;
  int argc;
  char **argv;
  enum {
    LINTERP_NO_HALT,
    LINTERP_BAD_SYNTAX,
    LINTERP_USER_EXIT
  } halt_type;
  union {
    char *error;
    int rc;
  } halt_value;
};


#endif
