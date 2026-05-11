// object.cpp
// Implements functions for creating and interning strings. String
// interning ensures that identical string literals share the
// same object. This allows fast comparison via pointer equality.

#include "object.h"

#include <functional>

// Compute the hash of a string using std::hash. In a more
// sophisticated implementation you might use a custom hash
// function. We store the result in ObjString::hash for reuse.
static std::size_t hashString(const std::string &str) {
    return std::hash<std::string>{}(str);
}

ObjString *allocateString(const std::string &chars,
                          std::vector<Obj *> &objects) {
    ObjString *stringObj = new ObjString;
    stringObj->type = ObjType::STRING;
    stringObj->next = nullptr;
    stringObj->chars = chars;
    stringObj->hash = hashString(chars);
    // Keep track of all allocated objects so they can be freed
    // later. In a real GC this would be used for marking.
    objects.push_back(reinterpret_cast<Obj *>(stringObj));
    return stringObj;
}

ObjString *copyString(const std::string &chars,
                      std::unordered_map<std::string, ObjString *> &interned,
                      std::vector<Obj *> &objects) {
    auto found = interned.find(chars);
    if (found != interned.end()) {
        // Return existing interned string.
        return found->second;
    }
    ObjString *stringObj = allocateString(chars, objects);
    interned.emplace(chars, stringObj);
    return stringObj;
}