#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "prompt.h"
#include "grammar.h"
#include "lval.h"
#include "builtin.h"

#ifdef _WIN32
#include <string.h>
/* windows support */
static char buf[2048];

char *readline(char *prompt) {
    fputs(prompt, stdout);
    fgets(buf, 2048, stdin);
    char *cpy = malloc(strlen(buf) + 1);
    strcpy(cpy, buf);

    cpy[strlen(cpy) - 1] = '\0';
    return cpy;
}

void add_history(char* unused) {}

#elif _ARCHLINUX
/* arch linux support */
#include <histedit.h>
#include <editline/readline.h>

#else
/* other linux / Mac support */
#include <editline/history.h>
#include <editline/readline.h>

#endif

grammar_elems elems;

void sigint_handler(int signum) {
    /* clean-up has to handled by SIGINT handler since we have while (1) */
    if ( signum == SIGINT ) {
        grammar_elems_destroy(&elems);
        printf("\nBye.\n");
        exit(0);
    }
}

void do_repl(void) {
    char *input;
    printf("lisper version %s\n", "0.0.0.1");
    printf("Anders Busch 2018\n");
    puts("Press Ctrl+c to Exit\n");

    mpc_result_t r;
    lval_t *val;

    grammar_elems_init(&elems);
    grammar_make_lang(&elems);
    signal(SIGINT, sigint_handler);

    while ( 1 ) {
        input = readline("lisper>>> ");

        add_history(input);

        if ( mpc_parse("<stdin>", input, elems.Lisper, &r) ) {
            val = lval_eval(lval_read(r.output));
#ifdef _DEBUG
            mpc_ast_print(r.output);
            putchar('\n');
#endif
            lval_println(val);
            lval_destroy(val);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input); 
    }

}


