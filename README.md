# Lox Bytecode Virtual Machine

This project implements a high‐performance bytecode virtual machine for the **Lox** programming language used in CPSC 323. Unlike the tree‑walk interpreter from Project 1, this implementation compiles Lox source code directly into a flat array of opcodes and executes them with a stack machine. Debug mode proves that the interpreter is a true VM by disassembling the bytecode into a linear listing.

## Building

Use a C++17 compiler such as `g++` to build the program.  On Linux or macOS you can run:

```sh
g++ -std=c++17 main.cpp scanner.cpp value.cpp object.cpp chunk.cpp debug.cpp compiler.cpp vm.cpp -o loxvm
```

This compiles all source files and produces an executable named `loxvm`.  No external libraries are required.

## Running

To execute a Lox script, pass the filename to the interpreter:

```sh
./loxvm sample.lox
```

The interpreter will compile the file and run it.  Runtime errors and compile‑time errors are printed to standard error.

## Disassembling Bytecode

To view the emitted bytecode without executing it, run the program with the `-d` flag followed by the source file:

```sh
./loxvm -d sample.lox
```

This prints a linear disassembly showing each opcode, its operands and the constant pool values, similar to:

```
== Disassembly: sample.lox ==
0000 0001 OP_CONSTANT     0 '3'
0002    | OP_PRINT
0003    | OP_RETURN
```

This output demonstrates that the compiler produces a flat list of instructions rather than an AST.

## How It Works

1. **Scanning** – The `Scanner` breaks the source code into tokens using maximal munch.  It recognizes single‑ and multi‑character operators, numbers, strings, identifiers and keywords.  Line numbers are recorded for error messages.
2. **Compilation** – The `Compiler` uses recursive descent with a Pratt parser to translate tokens directly into bytecode stored in a `Chunk`.  The compiler supports arithmetic and comparison operators, logical values, unary operators, grouping, variable declarations and assignments, `print` statements, `if/else` statements and `while` loops.  It emits opcodes such as `OP_CONSTANT`, `OP_ADD`, `OP_SUBTRACT`, `OP_JUMP`, `OP_JUMP_IF_FALSE` and `OP_LOOP`.  Global variables and string literals are stored in a constant pool.
3. **Execution** – The `VM` executes the bytecode using a simple fetch–decode–execute loop.  It maintains a value stack for operands, a table of global variables, and an interned string table so that each unique string is stored only once.  Operations pop their operands from the stack, perform the computation and push the result.  Control flow instructions change the instruction pointer to implement jumps and loops.  A runtime error halts execution and prints an error message.
4. **Disassembly** – The `debug` module can print a human‑readable listing of the bytecode.  It shows instruction offsets, line numbers and constant values.  This proves that the program is a true bytecode VM rather than a hidden AST interpreter.

## Implemented Features

* Arithmetic expressions: `+`, `-`, `*`, `/` with proper operator precedence.
* Comparison operators: `==`, `!=`, `<`, `<=`, `>`, `>=`.
* Logical values: `true`, `false`, `nil`.
* Unary operators: logical not (`!`) and negation (`-`).
* Grouping with parentheses.
* String literals and string concatenation using `+`.
* Variable declarations and assignments using the `var` keyword.
* Global variables stored in a hash table with runtime error for undefined variables.
* Control flow: `if`/`else` statements and `while` loops implemented with forward and backward jumps.
* `print` statements and expression statements.
* Debug mode (`-d`) to disassemble bytecode.

## Sample Program

An example Lox program exercising many features can be found in `sample.lox`:

```lox
// Sample Lox program testing arithmetic, strings, variables, if/else and loops
print 1 + 2 * 3;            // Prints 7
print "hello" + " world";  // Prints hello world

var a = 10;
print a;
a = a + 5;
print a;

var i = 0;
while (i < 3) {
    print i;
    i = i + 1;
}

if (a > 10) {
    print "a is large";
} else {
    print "a is small";
}
```

Running `./loxvm sample.lox` will compile and execute this script, printing the expected output.  Running `./loxvm -d sample.lox` will show the underlying bytecode instructions.
