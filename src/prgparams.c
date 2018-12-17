#include <stddef.h>
#include <string.h>
#include "prgparams.h"


int parse_prg_params(int argc, char **argv, struct lisper_params *params) {
    
    char *filename = NULL;
    for ( int i = 1; i < argc; ++i ) {
        if ( !( strstr(argv[i], "--") && strstr(argv[i], "-") ) ) {
            filename = argv[i];
            break;
        }
    }
    
    if ( filename == NULL ) {
        return 1;
    }

    params->filename = filename;

    return 0;
}

