// compiler.h
// Declares the compiler that translates Lox source code directly
// into bytecode. Unlike a traditional compiler, this one does
// not build an abstract syntax tree. Instead it uses recursive
// descent with precedence parsing to emit instructions into a
// Chunk as it parses. This approach follows the one in
// "Crafting Interpreters" but is implemented in C++.

#pragma once

#include <string>

#include "chunk.h"
#include "scanner.h"
#include "vm.h"

// Compile Lox source code into a new Chunk. The VM is passed
// so that the compiler can allocate interned strings via the
// VM's string table. Returns nullptr on error and prints
// diagnostics to stderr.
Chunk *compile(const std::string &source, VM &vm);