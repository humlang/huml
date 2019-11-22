## Module Definitions

A module a single translation unit. There two kinds of modules:

1. Meta-Module - defines operators, substitution rules, or may contain theorems.
2. Module - general purpose module that needs an associated meta-module

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

