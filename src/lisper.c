#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "grammar.h"
#include "lenv.h"
#include "lval.h"
#include "builtin.h"
#include "exec.h"
#include "mempool.h"

grammar_elems elems; /* grammar elems can be reused */
lenv_t *env = NULL; /* Global environment */
const size_t hash_size = 500;
struct mempool *lval_mp = NULL;


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
    lval_mp = mempool_init(sizeof(lval_t), 10000);
    env = lenv_new(hash_size);
    register_builtins(env);

    grammar_elems_init(&elems);
    grammar_make_lang(&elems);

    signal(SIGINT, signal_handler);
    atexit(exit_handler);

    if ( argc >= 2 ) {
        exec_filein(env, argc, argv);
    } else {
        exec_repl(env, elems);
    }

    return EXIT_SUCCESS;
}

