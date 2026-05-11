// value.cpp
// Implements helper functions for the Value type defined in value.h.
// Printing values and comparing values live here to keep
// vm.cpp simpler.

#include "value.h"
#include "object.h"

#include <iostream>

void printValue(const Value &value) {
    switch (value.type) {
    case ValueType::NUMBER:
        std::cout << value.as.number;
        break;
    case ValueType::BOOL:
        std::cout << (value.as.boolean ? "true" : "false");
        break;
    case ValueType::NIL:
        std::cout << "nil";
        break;
    case ValueType::OBJ: {
        // Currently the only object type is string. We add more cases later as needed.
        Obj *obj = value.as.obj;
        if (obj->type == ObjType::STRING) {
            ObjString *str = asString(value);
            std::cout << str->chars;
        } else {
            std::cout << "<object>"; // That is a fallback for any future object types like functions, classes, closures, or instances 
                                     // if I need the extra credit
        }
        break;
    }
    }
}

bool valuesEqual(const Value &a, const Value &b) {
    if (a.type != b.type) return false;
    switch (a.type) {
    case ValueType::NUMBER:
        return a.as.number == b.as.number;
    case ValueType::BOOL:
        return a.as.boolean == b.as.boolean;
    case ValueType::NIL:
        return true;
    case ValueType::OBJ:
        // Compare object pointers directly. Because strings are
        // interned, pointer equality is equivalent to string equality.
        return a.as.obj == b.as.obj;
    }
    return false; // Unreachable, here to satisfy compiler.
}