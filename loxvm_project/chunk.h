// chunk.h
// Defines the Chunk structure, which holds the emitted bytecode
// and the constant pool. Each instruction in the code vector is a
// single byte (an opcode or operand). Larger operands such as
// constant indices are emitted as two bytes. Lines are stored
// alongside code to aid debugging.

#pragma once

#include <cstdint>
#include <vector>

#include "value.h"

// Enumeration of the opcodes supported by our VM. These map to
// specific byte values. The order here matters because the
// compiler and disassembler rely on these numeric values.
enum class OpCode : uint8_t {
    OP_CONSTANT,
    OP_NIL,
    OP_TRUE,
    OP_FALSE,
    OP_POP,
    OP_GET_GLOBAL,
    OP_DEFINE_GLOBAL,
    OP_SET_GLOBAL,
    OP_EQUAL,
    OP_GREATER,
    OP_LESS,
    OP_ADD,
    OP_SUBTRACT,
    OP_MULTIPLY,
    OP_DIVIDE,
    OP_NOT,
    OP_NEGATE,
    OP_PRINT,
    OP_JUMP,
    OP_JUMP_IF_FALSE,
    OP_LOOP,
    OP_RETURN
};

// A chunk is a sequence of bytecode instructions with a parallel
// array of line numbers and a constant pool. The compiler emits
// bytecode into the chunk. The VM reads from it.
struct Chunk {
    std::vector<uint8_t> code;
    std::vector<Value> constants;
    std::vector<int> lines;

    // Append a single byte to the code stream along with the line
    // number it originates from.
    void write(uint8_t byte, int line) {
        code.push_back(byte);
        lines.push_back(line);
    }

    // Add a constant to the constant pool and return its index.
    size_t addConstant(const Value &value) {
        constants.push_back(value);
        return constants.size() - 1;
    }
};