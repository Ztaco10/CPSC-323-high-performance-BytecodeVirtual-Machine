// vm.h
// Defines the virtual machine that executes compiled Lox bytecode.
// The VM maintains a stack for operand storage, a table of
// interned strings, and a table of global variables. It also
// owns all heap‑allocated objects so they can be freed when the
// program terminates. The VM runs in a simple fetch–decode–
// execute loop over a Chunk of bytecode.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "chunk.h"
#include "object.h"
#include "value.h"

// Result of interpreting a chunk of bytecode. OK means the program
// ran successfully; COMPILE_ERROR and RUNTIME_ERROR are used to
// signal early termination. Only OK returns normally from
// interpret().
enum class InterpretResult {
    OK,
    COMPILE_ERROR,
    RUNTIME_ERROR
};

class VM {
public:
    VM();
    ~VM();

    // Execute a compiled chunk. Returns an InterpretResult.
    InterpretResult interpret(Chunk *chunk);

    // Free all allocated objects. Should be called after
    // interpretation to avoid memory leaks.
    void freeVM();

    // Interned string table and list of allocated objects. These
    // members are public so the compiler can allocate strings via
    // copyString(). The objects vector is only appended to; the
    // VM frees all objects at shutdown.
    std::unordered_map<std::string, ObjString *> strings;
    std::vector<Obj *> objects;

private:
    // Operand stack for the VM. This simple vector is used as a
    // stack. We reserve some space to avoid frequent reallocations.
    std::vector<Value> stack;

    // Points into the currently executing chunk's code. We use an
    // index rather than a pointer for safety.
    size_t ip;

    // The chunk currently being executed. The VM does not own
    // this chunk; the caller is responsible for its lifetime.
    Chunk *currentChunk;

    // Global variables table. Maps variable names (C++ strings)
    // to their current value. Uninitialized variables are not
    // present in the map.
    std::unordered_map<std::string, Value> globals;

    // Stack manipulation helpers.
    void resetStack();
    void push(const Value &value);
    Value pop();
    Value peek(int distance) const;

    // Error reporting helper. Prints a runtime error message and
    // resets the stack. Returns InterpretResult::RUNTIME_ERROR.
    InterpretResult runtimeError(const std::string &message);

    // Execute one instruction. Returns false to halt the VM.
    bool runInstruction();

    // Concatenate the top two string values on the stack. Both
    // operands must be ObjString; returns a new interned ObjString.
    void concatenate();
};