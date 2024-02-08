
# Lisper Language Reference

## Lisper Language Constituents

The Lisper interpreter implements a dialect of the Lisp language that includes:

- symbolic expressions (aka. s-exprs)
- quoted expressions (aka. q-exprs)
- symbols
- integers
- double precision floating points
- byte sized strings

In lisp, expressions are collection of symbols delimited by whitespace.
Syntactically, symbolic expressions are expressions enclosed by parentheses, while quoted expressions are enclosed by curly braces. 

Symbolic expressions are automatically evaluated, depth-first, with the first symbol in symbolic expression counting as the operator. The operator decides the semantics of the s-expr evaluation.

Quoted expressions are not automatically evaluated. This makes them suitable to define lists containing data, function body declarations or constructs which requires evaluation at a later date.

## Operators

### Arithmetic operators

- addition +
- subtraction -
- multiplications *
- division /
- modulus %

As all of these operators are binary operators, these take at least 2 input values and are defined for integers and floating points.

When additional input values are given, these are successively applied to the intermediated results from the application of the operator to the 2 previous input values.
This means that the result of the expression 
`(/ 100 2 3)` and
`(/ (/ (/ 100 2) 3))` are both `16`.

## Builtins

