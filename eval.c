#include "eval.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

lval_t lval_num(double num) {
    lval_t val;
    val.type = LVAL_NUM;
    val.val.num = num;
    return val;
}

lval_t lval_err(lerr_t err) {
    lval_t val;
    val.type = LVAL_ERR;
    val.val.err = err;
    return val;
}

void lval_print(lval_t *val) {

    switch ( val->type ) {
        case LVAL_NUM:
            printf("%lf", val->val.num);
            break;
        case LVAL_ERR:
            switch ( val->val.err ) {
                case LERR_DIV_ZERO:
                    printf("Error: Division by zero");
                    break;
                case LERR_BAD_OP:
                    printf("Error: Invalid operator");
                    break;
                case LERR_BAD_NUM:
                    printf("Error: Invalid number");
                    break;
                default:
                    printf("Error: Unknown error");
                    break;
            }
            break;
    }
}

void lval_println(lval_t *val) {
    lval_print(val);
    putchar('\n');
}

lval_t eval_op(char *op, lval_t x, lval_t y) {

    if ( x.type == LVAL_ERR ) {
        return x;
    }
    if ( y.type == LVAL_ERR ) {
        return y;
    }

    if ( strcmp(op, "+") == 0 ) {
        return lval_num(x.val.num + y.val.num);
    } else if ( strcmp(op, "-") == 0 ) {
        return lval_num(x.val.num - y.val.num);
    } else if ( strcmp(op, "*") == 0 ) {
        return lval_num(x.val.num * y.val.num);
    } else if ( strcmp(op, "/") == 0 ) {
        return y.val.num == 0.0 ?
            lval_err(LERR_DIV_ZERO) :
            lval_num(x.val.num / y.val.num);
    } else if ( strcmp(op, "%") == 0 ) {
        return y.val.num == 0.0 ?
            lval_err(LERR_DIV_ZERO) :
            lval_num(fmod(x.val.num, y.val.num));
    } else if ( strcmp(op, "^") == 0 ) {
        return lval_num(pow(x.val.num, y.val.num));
    } else if ( strcmp(op, "min") == 0  ) {
        return x.val.num <= y.val.num ? x : y;
    } else if ( strcmp(op, "max") == 0 ) {
        return x.val.num >= y.val.num ? x : y;
    }
    return lval_err(LERR_BAD_OP);
}

lval_t eval(mpc_ast_t *t) {
    lval_t val;
    double num;
    char *op;
    int i, code;

    if ( strstr(t->tag, "number") ) {
        code = sscanf(t->contents, "%lf", &num);
        if ( code == 1 ) {
            val = lval_num(num);
            return val;
        }
        return lval_err(LERR_BAD_NUM);
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

