#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "prgparams.h"
#include "lisper.h"

int parse_prg_params(int argc, char **argv, struct lisper_params *params) {
    
    char *filename = NULL;
    int version = 0;
    int help = 0;
    int followed_by_optional = 0; /* bool trigger for options that take arguments */
    int arg_count = 0;

    char *current;
    for ( int i = 1; i < argc; ++i ) {
        current = argv[i];
        if ( !( strstr(current, "--") && strstr(current, "-") ) && !followed_by_optional ) {
            arg_count++;
            filename = current;
            break; /* program filename found stopping lisper argument parsing  */
        } else {
            if ( strcmp(current, "--help") == 0 || strcmp(current, "-h") == 0 ) {
                help = 1;
                arg_count++;
            } else if ( strcmp(current, "--version") == 0 || strcmp(current, "-v") == 0 ) {
                version = 1;
                arg_count++;
            }
        }
    }

    params->filename = filename;
    params->version = version;
    params->help = help;
    params->arg_count = arg_count;

    return 0;
}

int handle_prg_params(struct lisper_params *parsed_params) {
    
    if (parsed_params->version) {
        printf(
            "Lisper version %s\n",
            LISPER_VERSION
        );
        exit(0);
    }


    return 0;
}

