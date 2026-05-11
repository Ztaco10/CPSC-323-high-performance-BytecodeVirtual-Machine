// object.h
// Defines the base object types for the VM. The VM stores values
// as pointers to Obj. For Project 2, the only object type is
// ObjString. In the future, you could add functions, closures,
// classes, etc. Each object has a type tag so the VM knows how
// to handle it.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

// Forward declare Value so we can reference it in objects if
// needed. (Not required for strings but useful later.)
struct Value;

// Enumeration of all heap‑allocated object types. Only strings are
// required for Project 2. Additional types like functions and
// closures would come later.
enum class ObjType {
    STRING
};

// Base struct for every heap‑allocated object. We could put
// bookkeeping fields here (e.g., for garbage collection), but
// Project 2 does not implement GC. Each object stores its type
// so the VM can downcast appropriately.
struct Obj {
    ObjType type;
    // Optional: pointer to next object for GC. Not used here.
    Obj *next;
};

// A string object holds a std::string. Lox strings are immutable.
// We also store the precomputed hash to speed up table lookups.
struct ObjString : Obj {
    std::string chars;
    std::size_t hash;
};

// Create a new ObjString from a C++ string. The VM passes the
// interned string table so we can return an existing string if
// it has already been allocated. The function returns a
// pointer to the interned ObjString.
ObjString *copyString(const std::string &chars,
                      std::unordered_map<std::string, ObjString *> &interned,
                      std::vector<Obj *> &objects);

// Allocate a raw ObjString without interning. Used internally by
// copyString. Takes ownership of the string. Keeps track of
// allocated objects so they can be freed on VM shutdown.
ObjString *allocateString(const std::string &chars,
                          std::vector<Obj *> &objects);