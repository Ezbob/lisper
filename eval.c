#include "eval.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

double eval_op(char *op, double x, double y) {
    if ( strcmp(op, "+") == 0 ) {
        return x + y;
    } else if ( strcmp(op, "-") == 0 ) {
        return x - y;
    } else if ( strcmp(op, "*") == 0 ) {
        return x * y;
    } else if ( strcmp(op, "/") == 0 ) {
        return x / y;
    } else if ( strcmp(op, "^") == 0 ) {
        return pow(x, y); 
    } else if ( strcmp(op, "min") == 0  ) {
        return x <= y ? x : y;
    } else if ( strcmp(op, "max") == 0 ) {
        return x >= y ? x : y;
    }
    return 0.0;
}

double eval(mpc_ast_t *t) {
    double val;
    char *op;
    int i;

    if ( strstr(t->tag, "number") ) {
        sscanf(t->contents, "%lf", &val);
        return val;
    }
    
    op = t->children[1]->contents;
    val = eval(t->children[2]);
    i = 3;

    while ( strstr(t->children[i]->tag, "expr") ) {
        val = eval_op(op, val, eval(t->children[i]));
        i++;
    }

    return val;
}

