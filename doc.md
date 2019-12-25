## Statements

Statements are high-level constructs in the programming language that do computations.
They do not have a type or a value.

### Syntax

```
stmt ::= loop-stmt | block-stmt | seq-stmt | assign-stmt | print-stmt | read-stmt
```

## Loops

We only allow loops of constant, copile-time known size.
For example, `for 5 { print("hello\n") }` prints "hello\n" five times.
It is currently not possible to do `for x { /* ... */ }` due to the parsing.
However, this is planned for the future.

### Syntax

```
loop-stmt ::= `for` number block-stmt
```

## Blocks

Blocks group statements. They do not "return" a value, but may alter arbitrary state.
Variables inside a block only live inside that block, i.e. they *can't* be referenced
from the outside as such:

```
{
  x := 5
} ;
print(x) // error!
```

### Syntax

```
block-stmt ::= `{` stmt `}`
```

## Sequentialization

The sequentialization allows to execute statements one after another.
Contrary to C or C++, we do not use it to simply end the statement and explicitly
require it to chain statements.

### Syntax

```
seq-stmt ::= stmt | seq-stmt ';' seq-stmt
```

## Asign-Statement

Assignments may be used to store values of expressions.

### Syntax

```
assign-stmt ::= identifier `:=` expression
```

## Print-Statement

In order to print values, one may use the dedicated `print` function.
It is an intrinsic function that is currently implemented as keyword into the language.

### Syntax

```
print-stmt ::= `print(` identifier `)`
```

## Read-Statement

Similarily to printing values, we may read values. Note that values are integers.

### Syntax

```
read-stmt ::= `read(` identifier `)`
```

<!--

## Module Definitions

A module a single translation unit. There are two kinds of modules:

1. Meta-Module - defines operators, substitution rules, or may contain theorems.
2. Module - general purpose module that needs an associated meta-module.

The filename determines the name of the module.
A file with the extension `.hl` is a general purpose module.
A file with the extension `.hlm` is a meta-module.
Note that a file name should not contain more than one dot. To mitigate this issue
and allow modules to be named with dots in code, e.g. `math.analysis.differentiation`, 
every `/` in the module's filename will be replaced with `.`.
Hence, the module `math.analysis.differentiation` is uniquely defined by the file
`differentiation.hl` contained in the directory `analysis`, which again is contained
in the directory `math`:

```
math
└── analysis
    └── differentiation.hl
```

### Import

An import loads a (meta-)module.
For this, the compiler first looks at it's current working directory and loads any
module matching the name provided to `import`. If it can't find any module in there
(including subdirectiories), it will look at the `PATH` variable's entries.
There is also the possibility to provide the compiler with import-directories <!-- TODO: add a link to reference this -->
where it will additionally look for if the local directory wasn't enough.
Overall, the lookup is as follows: (where each `->` means that the module wasn't found)

```
  [Local]   ->    [Import-Directories]    ->    [PATH-Directories]
```

If no specific module is referenced, e.g. `import math.analysis` transitively imports
all modules found in `math/analysis/`.

###### Syntax

```
import-thing ::= `import` identifier-with-dot
```

###### Examples

```
import math.trigonometry;
```



### Use


###### Examples

```
foo() -> () use math.analysis in { return 0; }
```

-->

