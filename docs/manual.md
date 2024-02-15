
# Lisper Language Reference

## Lisper Language Constituents

The Lisper interpreter implements a dialect of the Lisp language that includes:

- symbolic expressions (aka. s-exprs)
- quoted expressions (aka. q-exprs)
- symbols
- integers
- double precision floating points
- byte sized strings

<!--
In lisp, expressions are collection of symbols delimited by whitespace.
Syntactically, symbolic expressions are expressions enclosed by parentheses, while quoted expressions are enclosed by curly braces. 

Symbolic expressions are automatically evaluated, depth-first, with the first symbol in symbolic expression counting as the operator. The operator decides the semantics of the s-expr evaluation.

Quoted expressions are not automatically evaluated. This makes them suitable to define lists containing data, function body declarations or constructs which requires evaluation at a later date.

-->
## Basic syntax

### Symbolic expressions

The basic syntax for symbolic expressions is as follows:

```(function_name expressions...)```

or 
```function_name expressions...``` for single root expression.

The `function_name` designates the operation to be done, and the series of `expressions` designates the inputs that `function_name` is evaluated on.

Each symbolic expression may return one value or the empty symbolic expression `()`.

Any input `expression` to an symbolic expression may be an literal or another symbolic expression. An symbolic expression that exists as a input expression to another symbolic expression is called a nested symbolic expression.

During evaluation of symbolic expressions, nested symbolic expression are evaluated first, left-to-right, replacing their return value as the input to the outer symbolic expression before the outer symbolic expression. 

### Quoted expressions

Quoted expressions are defined much the same as symbolic expressions, except that quoted expressions are expressed using curly brackets ( "{" and "}" ). 

Contrary to Symbolic expression are not evaluated, but can be converted to and from symbolic expressions, using builtins. Because quoted expression are not evaluated, they can be used to model data structures such as lists. 

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

## Built-in functions (Builtins)

