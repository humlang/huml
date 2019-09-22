
# h-lang specification

## Syntax

```
Expr     ::= CallExpr
           | IdExpr
           | NatExpr

NatExpr  ::= Nat | NNat NatExpr
Nat      ::= 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
NNat     ::= 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

IdExpr   ::= Id
Id       ::= [A-Za-z+-~/*%]+

CallExpr ::= Id Arg-list

Arg-list ::= Arg
           | Arg `,` Arg-list

Arg      ::= Expr
```

## Semantics


# h-lang IR

## Types

```
type ::= i{N}
       | u{N}
       | f32
       | f64
       | d32
       | d64
       | ex A. type
       | mu A. type
       | A
       | ref <type ...>
       | box H
```
{N} specifies a natural number unequal zero.

## Syntax

```

```
