// debug.cpp
// Implements the disassembly functions declared in debug.h. The
// disassembler walks through a chunk's code array and prints
// each instruction with its operands. For multi‑byte operands
// (constant indices and jump offsets) we read two bytes and
// combine them into a 16‑bit number. See Chapter 17 of
// Crafting Interpreters for details on decoding bytecode.

#include "debug.h"

#include <iomanip>
#include <iostream>

// Helper to print a constant instruction. Reads the next byte
// (or two bytes for larger indices) and prints the constant
// value. Returns the offset after the operand.
static size_t constantInstruction(const std::string &name, const Chunk *chunk, size_t offset) {
    uint8_t index = chunk->code[offset + 1];
    std::cout << std::left << std::setw(16) << name << " "
              << std::setw(4) << (int)index << " '";
    printValue(chunk->constants[index]);
    std::cout << "'\n";
    return offset + 2;
}

// Helper to print a jump instruction. A jump instruction has a
// 16‑bit operand stored in the next two bytes. 'sign' is +1 for
// forward jumps and ‑1 for backward loops.
static size_t jumpInstruction(const std::string &name, int sign, const Chunk *chunk, size_t offset) {
    uint16_t jump = static_cast<uint16_t>(chunk->code[offset + 1] << 8);
    jump |= chunk->code[offset + 2];
    std::cout << std::left << std::setw(16) << name << " "
              << offset << " -> " << (offset + 3 + sign * jump) << "\n";
    return offset + 3;
}

void disassembleChunk(const Chunk *chunk, const char *name) {
    std::cout << "== Disassembly: " << name << " ==\n";
    for (size_t offset = 0; offset < chunk->code.size();) {
        offset = disassembleInstruction(chunk, offset);
    }
}

size_t disassembleInstruction(const Chunk *chunk, size_t offset) {
    std::cout << std::setw(4) << std::setfill('0') << offset << " ";
    int line = chunk->lines[offset];
    if (offset > 0 && line == chunk->lines[offset - 1]) {
        std::cout << "   | ";
    } else {
        std::cout << std::setw(4) << line << " ";
    }
    std::cout << std::setfill(' ');
    uint8_t instruction = chunk->code[offset];
    switch (static_cast<OpCode>(instruction)) {
    case OpCode::OP_CONSTANT:
        return constantInstruction("OP_CONSTANT", chunk, offset);
    case OpCode::OP_NIL:
        std::cout << "OP_NIL" << "\n";
        return offset + 1;
    case OpCode::OP_TRUE:
        std::cout << "OP_TRUE" << "\n";
        return offset + 1;
    case OpCode::OP_FALSE:
        std::cout << "OP_FALSE" << "\n";
        return offset + 1;
    case OpCode::OP_POP:
        std::cout << "OP_POP" << "\n";
        return offset + 1;
    case OpCode::OP_GET_GLOBAL:
        return constantInstruction("OP_GET_GLOBAL", chunk, offset);
    case OpCode::OP_DEFINE_GLOBAL:
        return constantInstruction("OP_DEFINE_GLOBAL", chunk, offset);
    case OpCode::OP_SET_GLOBAL:
        return constantInstruction("OP_SET_GLOBAL", chunk, offset);
    case OpCode::OP_EQUAL:
        std::cout << "OP_EQUAL" << "\n";
        return offset + 1;
    case OpCode::OP_GREATER:
        std::cout << "OP_GREATER" << "\n";
        return offset + 1;
    case OpCode::OP_LESS:
        std::cout << "OP_LESS" << "\n";
        return offset + 1;
    case OpCode::OP_ADD:
        std::cout << "OP_ADD" << "\n";
        return offset + 1;
    case OpCode::OP_SUBTRACT:
        std::cout << "OP_SUBTRACT" << "\n";
        return offset + 1;
    case OpCode::OP_MULTIPLY:
        std::cout << "OP_MULTIPLY" << "\n";
        return offset + 1;
    case OpCode::OP_DIVIDE:
        std::cout << "OP_DIVIDE" << "\n";
        return offset + 1;
    case OpCode::OP_NOT:
        std::cout << "OP_NOT" << "\n";
        return offset + 1;
    case OpCode::OP_NEGATE:
        std::cout << "OP_NEGATE" << "\n";
        return offset + 1;
    case OpCode::OP_PRINT:
        std::cout << "OP_PRINT" << "\n";
        return offset + 1;
    case OpCode::OP_JUMP:
        return jumpInstruction("OP_JUMP", 1, chunk, offset);
    case OpCode::OP_JUMP_IF_FALSE:
        return jumpInstruction("OP_JUMP_IF_FALSE", 1, chunk, offset);
    case OpCode::OP_LOOP:
        return jumpInstruction("OP_LOOP", -1, chunk, offset);
    case OpCode::OP_RETURN:
        std::cout << "OP_RETURN" << "\n";
        return offset + 1;
    default:
        std::cout << "Unknown opcode " << (int)instruction << "\n";
        return offset + 1;
    }
}