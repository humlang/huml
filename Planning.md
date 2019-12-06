h-lang - Road to Version 0.0.1 (alpha)



#### Step 1 to conquering the world

- [ ] FOR (as in basic theoretical cs course) AST node     # for 5 { /*runs 5 times*/ }
- [ ] basic arithmetic expressions
  - [ ] parsing of +, -, *     (Precedence Climbing)
  - [ ] nodes for binary expressions
  - [ ] node for assignment expression/statement
- [ ] print AST node (-> intrinsic function)
- [ ] read AST node (-> intrinsic function,    `getchar`)
- [ ] AST visitor (similar to ast_print) which evaluates    aka "eval per node"
- [ ] add block AST node
- [ ] parse block AST node



### Step 2 to CtW

- [ ] write an emitter that emits C code from the AST
- [ ] implement SSA conversion by Borsche et. al
- [ ] perform trivial optimizations (i.e. loop unrolling, expression folding)
- [ ] Implement a basic register VM   (ADD, SUB, PRINT, READ, BEQZ)  (register count as in x86_64)
- [ ] Add a stack to the VM   (PUSH, POP)
- [ ] Implement an emitter that emits VM Instructions
- [ ] abstraction for lower level assembly stuff to just deal with that on high-level. perhaps use ASMJit
- [ ] Control Flow analysis
- [ ] Data Flow analysis



### Step 3 to CtW

- [ ] implement Tseitin's heuristic for register allocation    (register = thing in the vm)
  - [ ] abstraction for graphs that can efficiently give back the node with lowest degree
  - [ ] abstraction for lower level machine stuff
  - [ ] actual algorithm of tseitin with spilling
  - [ ] OPTIONAL: try to avoid spilling inside of loop
- [ ] Change VM Instructions from Text/Enums to compressed bytecode
  - [ ] Write an assembler, i.e. a thing that reads your VM instr as text and converts to bytecode
- [ ] Think about and implement an executable format, i.e. ELF-like
- [ ] Change emittance of VM instructions to the bytecode  (perhaps this is easy with the abstraction)
- [ ] Write a basic linker that is able to resolve referenced variables in other modules
  - [ ] Implement modules, i.e. let the compiler for now just combine the AST from several files
  - [ ] Thoroughly think about how to do modules efficiently (-> optimize speed of compilation)











