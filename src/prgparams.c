#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "prgparams.h"
#include "lisper.h"

void exit_with_help(int exit_code) {
    printf(
            "Usage: lisper [OPTION]... [FILE]\n"
            "Run a lisper program specified by FILE or enter a REPL (Read-Eval-Print Loop) session if no FILE is given.\n"
            "\n"
            "OPTIONs available:\n"
            "  -v, --version            show version infomation and exit\n"
            "  -h, --help               show this message and exit\n"
            "  -c <COMMAND>             run <COMMAND> and exit\n"
            "\n"
            "Lisper online source code repository: <https://www.github.com/Ezbob/lisper>\n"
            "Licensed under the very permissive MIT license\n" 
          );
    exit(exit_code);
}

int parse_prg_params(int argc, char **argv, struct lisper_params *params) {
    
    char *filename = NULL;
    char *command = NULL;
    int version = 0;
    int help = 0;
    int followed_by_optional = 0; /* bool trigger for options that take arguments */
    int arg_count = 0;
    char *current;

    for ( int i = 1; i < argc; ++i ) {
        current = argv[i];
        if ( strlen(current) > 0 && current[0] != '-' && !followed_by_optional ) {
            arg_count++;
            filename = current;
            break; /* program filename found stopping lisper argument parsing  */
        } else if ( strlen(current) > 0 ) {
            /* insert optional parse conditions here */
            if ( strcmp(current, "--help") == 0 || strcmp(current, "-h") == 0 ) {
                help = 1;
                arg_count++;
            } else if ( strcmp(current, "--version") == 0 || strcmp(current, "-v") == 0 ) {
                version = 1;
                arg_count++;
            } else if ( strcmp(current, "-c") == 0 ) {
                arg_count++;
                if ((i + 1) >= argc) {
                    return 1;
                }
                i += 1;
                char *value = argv[i];
                if (strlen(value) == 0) {
                    return 1;
                }
                command = value;
            } else {
                return 1;
            }
        }
    }

    params->command = command;
    params->filename = filename;
    params->version = version;
    params->help = help;
    params->arg_count = arg_count;

    return 0;
}

int handle_prg_params(struct lisper_params *parsed_params) {
    
    if (parsed_params->help) {
        exit_with_help(0);
    }

    if (parsed_params->version) {
        printf(
            "Lisper version %s\n",
            LISPER_VERSION
        );
        exit(0);
    }

    return 0;
}

