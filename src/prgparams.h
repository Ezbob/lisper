#ifndef LISPER_PRGPARAMS
#define LISPER_PRGPARAMS

struct lisper_params {
    char *filename;
    int help;
    int version;
    int arg_count;
};

int parse_prg_params(int, char **, struct lisper_params *);

int handle_prg_params(struct lisper_params *);

#endif

