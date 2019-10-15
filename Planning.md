
# h-lang specification

## Syntax

```
Expr     ::= CallExpr
           | IdExpr
           | NatExpr

NatExpr  ::= Nat | NNat Nat+
Nat      ::= 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9
NNat     ::= 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9

IdExpr   ::= Id
Id       ::= [A-Za-z+-~/*%<>]+

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
```
{N} specifies a natural number unequal zero.

## Syntax

```

```
