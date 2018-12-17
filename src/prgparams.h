#ifndef LISPER_PRGPARAMS
#define LISPER_PRGPARAMS

struct lisper_params {
    char *filename;
};

int parse_prg_params(int argc, char **argv, struct lisper_params *params);


#endif

