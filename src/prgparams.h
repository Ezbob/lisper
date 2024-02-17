#ifndef LISPER_PRGPARAMS
#define LISPER_PRGPARAMS

struct lisper_params {
  char *filename;
  char *command;
  int help;
  int version;
  int arg_count;
};

void exit_with_help(int);

int parse_prg_params(int, char **, struct lisper_params *);

int handle_prg_params(struct lisper_params *);

#endif
