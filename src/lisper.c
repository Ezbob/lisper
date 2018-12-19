#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "lisper.h"
#include "grammar.h"
#include "lenv.h"
#include "lval.h"
#include "builtin.h"
#include "exec.h"
#include "mempool.h"
#include "prgparams.h"

grammar_elems elems; /* grammar elems can be reused */
lenv_t *env = NULL; /* Global environment */
const size_t hash_size = 500;
struct mempool *lval_mp = NULL;
const size_t lval_mempool_size = 10000;
struct argument_capture *args;


void signal_handler(int signum) {
    /* clean-up has to handled by SIGINT handler since we have while (1) */
    if ( signum == SIGINT ) {
        exit(0);
    } else {
        grammar_elems_destroy(&elems);
        lenv_del(env);
    }
}

void exit_handler(void) {
    grammar_elems_destroy(&elems);
    lenv_del(env);
    mempool_del(lval_mp);
}

int main(int argc, char **argv) {

    struct argument_capture capture;
    struct lisper_params params;

    capture.argc = argc;
    capture.argv = argv;

    args = &capture;

    lval_mp = mempool_init(sizeof(lval_t), lval_mempool_size);
    env = lenv_new(hash_size);
    register_builtins(env);

    grammar_elems_init(&elems);
    grammar_make_lang(&elems);

    signal(SIGINT, signal_handler);
    atexit(exit_handler);
    
    if ( parse_prg_params(argc, argv, &params) != 0 ) {
        fprintf(stderr, "Error: Couldn't parse lisper arguments\n");
        exit(1);
    }

    if ( handle_prg_params(&params) != 0 ) {
        fprintf(stderr, "Error: Encountered error in handling lisper arguments\n");
        exit(1);
    }

    if ( params.filename != NULL ) {        
        exec_filein(env, &params);
    } else {
        exec_repl(env, elems);
    }

    return EXIT_SUCCESS;
}

