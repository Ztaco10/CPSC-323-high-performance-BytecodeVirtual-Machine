// debug.h
// Provides functions for disassembling bytecode in a Chunk.
// The disassembler prints each instruction in a human‑readable
// form along with its offset and any operands. This is used in
// debug mode to verify that the compiler emitted a flat list of
// opcodes, proving that we built a real bytecode VM.

#pragma once

#include "chunk.h"

// Disassemble an entire chunk of bytecode. 'name' is printed at
// the top of the disassembly for context (usually the filename).
void disassembleChunk(const Chunk *chunk, const char *name);

// Disassemble a single instruction at the given offset. Returns
// the offset of the next instruction. This helper is used by
// disassembleChunk.
size_t disassembleInstruction(const Chunk *chunk, size_t offset);