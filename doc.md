## Module Definitions

A module a single translation unit. There two kinds of modules:

1. Meta-Module - defines operators, substitution rules, or may contain theorems.
2. Module - general purpose module that needs an associated meta-module

###### Syntax

```
module-init ::= `[` meta-module? module-name `]` `;` module-translation-unit
module-name ::= identifier-with-dot
meta-module ::= `meta`
```

A module may only be defined once in the whole translation.

###### Examples

```
[math.trigonometry]
```

```
[meta math.analysis]
```



### Usage

###### Syntax

```
use-thing ::= `use` identifier-with-dot use-specification
use-specification ::= `in` fn-block | `globally` `;`
```



###### Examples

```
use math.trigonometry globally;
```

```
foo() -> () use math.analysis in { return 0; }
```

