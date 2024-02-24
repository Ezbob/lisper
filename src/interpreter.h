
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
};

#endif
