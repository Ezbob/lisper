#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "lisper.h"
#include "grammar.h"
#include "environment.h"
#include "value.h"
#include "builtin.h"
#include "execute.h"
#include "mempool.h"
#include "prgparams.h"

struct grammar_elems elems; /* grammar elems can be reused */
struct lenvironment *env = NULL; /* Global environment */
const size_t hash_size = 500;
struct mempool *lvalue_mp = NULL;
const size_t lvalue_mempool_size = 10000;
struct argument_capture *args;


void signal_handler(int signum) {
    /* clean-up has to handled by SIGINT handler since we have while (1) */
    if ( signum == SIGINT ) {
        exit(0);
    } else {
        grammar_elems_destroy(&elems);
        lenvironment_del(env);
    }
}

void exit_handler(void) {
    grammar_elems_destroy(&elems);
    lenvironment_del(env);
    mempool_del(lvalue_mp);
}

int main(int argc, char **argv) {

    struct argument_capture capture;
    struct lisper_params params;

    capture.argc = argc;
    capture.argv = argv;

    args = &capture;

    lvalue_mp = mempool_init(sizeof(struct lvalue), lvalue_mempool_size);
    env = lenvironment_new(hash_size);
    register_builtins(env);

    grammar_elems_init(&elems);
    grammar_make_lang(&elems);

    signal(SIGINT, signal_handler);
    atexit(exit_handler);
    
    if ( parse_prg_params(argc, argv, &params) != 0 ) {
        fprintf(stderr, "Error: Couldn't parse lisper program arguments\n");
        exit_with_help(1);
    }

    if ( handle_prg_params(&params) != 0 ) {
        fprintf(stderr, "Error: Encountered error in handling lisper arguments\n");
        exit_with_help(1);
    }

    if ( params.filename != NULL ) {        
        exec_filein(env, &params);
    } else {
        exec_repl(env, &elems);
    }

    return EXIT_SUCCESS;
}

