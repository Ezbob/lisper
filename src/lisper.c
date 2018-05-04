#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include "grammar.h"
#include "lenv.h"
#include "lval.h"
#include "builtin.h"
#include "prompt.h"

grammar_elems elems; /* grammar elems can be reused */
lenv_t *env = NULL; /* Global environment */

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
    putchar('\n');
}

int main(int argc, char **argv) {
    env = lenv_new();
    lenv_add_builtins(env);

    grammar_elems_init(&elems);
    grammar_make_lang(&elems);

    signal(SIGINT, signal_handler);
    atexit(exit_handler);

    if ( argc >= 2 ) {

        for ( int i = 1; i < argc; ++i ) {
            lval_t *args = lval_add(lval_sexpr(), lval_str(argv[i]));
            lval_t *x = builtin_load(env, args);
            if ( x->type == LVAL_ERR ) {
                lval_println(x);
            }
            lval_del(x);
        }

    } else {
        do_repl(env, elems);
    }

    return EXIT_SUCCESS;
}

