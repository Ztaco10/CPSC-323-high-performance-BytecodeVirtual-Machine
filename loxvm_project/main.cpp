// main.cpp
// Entry point for the Lox bytecode virtual machine.
// This file parses command‑line arguments, reads a source file
// and either disassembles or executes it. The VM itself lives
// in vm.cpp and compiler.cpp. We keep main small so you can
// quickly see how the pieces fit together.

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "compiler.h"
#include "debug.h"
#include "vm.h"

// Read an entire file into a single string. Returns false on failure.
static bool readFile(const std::string &path, std::string &out) {
    std::ifstream file(path, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    out = buffer.str();
    return true;
}

int main(int argc, char **argv) {
    bool debugMode = false;
    std::string filename;

    // Simple command line parsing. We accept either:
    //  ./loxvm script.lox
    // or
    //  ./loxvm -d script.lox    (disassemble bytecode only)
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0]
                  << " [-d] <script.lox>\n";
        return 1;
    }
    int argIndex = 1;
    if (std::string(argv[argIndex]) == "-d") {
        debugMode = true;
        argIndex++;
    }
    if (argIndex >= argc) {
        std::cerr << "Missing source file.\n";
        return 1;
    }
    filename = argv[argIndex];

    std::string source;
    if (!readFile(filename, source)) {
        std::cerr << "Could not read file: " << filename << "\n";
        return 1;
    }

    VM vm;
    // Compile the source into a chunk. The compiler lives in
    // compiler.cpp and produces a Chunk on success.
    Chunk *chunk = compile(source, vm);
    if (chunk == nullptr) {
        // Compilation failed; errors have already been printed.
        return 1;
    }

    if (debugMode) {
        // In debug mode, disassemble the chunk without executing it.
        disassembleChunk(chunk, filename.c_str());
    } else {
        // Otherwise run the compiled bytecode using our VM.
        InterpretResult result = vm.interpret(chunk);
        if (result != InterpretResult::OK) {
            // Execution errors are printed inside the VM.
            return 1;
        }
    }

    // Clean up memory owned by the VM (strings, etc.).
    vm.freeVM();
    delete chunk;
    return 0;
}