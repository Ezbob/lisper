
# Lisper Interpreter

## Language Reference

### Lisper Language Constituents

The Lisper interpreter implements a dialect of the Lisp langauge that includes:

- symbolic expressions (aka. s-exprs)
- quoted expressions (aka. q-exprs)
- symbols
- integers
- double precision floating points
- strings

In lisp, expressions are collection of symbols delimited by whitespace.
Syntactically, symbolic expressions are expressions enclosed by parentheses, while quoted expressions are enclosed by curly braces. 

Symbolic expressions are automatically evaluated, depth-first, with the first symbol in symbolic expression counting as the operator. The operator decides the semantics of the s-expr evaluation.

Quoted expressions are not automatically evaluated. This makes them suitable to define lists containing data, function body declarations or constructs which requires evaluation at a later date.

### Builtins
****

