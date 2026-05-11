// value.h
// Defines the dynamic Value type used by the VM. Each Value
// holds a number, boolean, nil, or pointer to an object. Since
// Lox is dynamically typed, all runtime values are boxed in a
// single structure. Helper functions are provided to test and
// extract the underlying data.

#pragma once

#include <cstdint>
#include <iostream>

// Forward declarations for objects. See object.h for details.
struct Obj;
struct ObjString;

// The type tag for a Value. Values can be numbers, booleans,
// nil, or object pointers. Additional types could be added
// later (functions, closures, etc.).
enum class ValueType {
    NUMBER,
    BOOL,
    NIL,
    OBJ
};

// A dynamically typed value. We use a simple tagged union.
// When type == NUMBER, the number field is valid.
// When type == BOOL, the boolean field is valid.
// When type == OBJ, the obj field is a pointer to a heap object.
struct Value {
    ValueType type;
    union {
        double number;
        bool boolean;
        Obj *obj;
    } as;
};

// Utility constructors for values.
inline Value makeNumber(double num) {
    Value v;
    v.type = ValueType::NUMBER;
    v.as.number = num;
    return v;
}

inline Value makeBool(bool b) {
    Value v;
    v.type = ValueType::BOOL;
    v.as.boolean = b;
    return v;
}

inline Value makeNil() {
    Value v;
    v.type = ValueType::NIL;
    return v;
}

inline Value makeObj(Obj *obj) {
    Value v;
    v.type = ValueType::OBJ;
    v.as.obj = obj;
    return v;
}

// Predicates to check the type of a value.
inline bool isNumber(const Value &v) { return v.type == ValueType::NUMBER; }
inline bool isBool(const Value &v) { return v.type == ValueType::BOOL; }
inline bool isNil(const Value &v) { return v.type == ValueType::NIL; }
inline bool isObj(const Value &v) { return v.type == ValueType::OBJ; }

// Accessors for values. Caller must ensure the type is correct.
inline double asNumber(const Value &v) { return v.as.number; }
inline bool asBool(const Value &v) { return v.as.boolean; }
inline Obj *asObj(const Value &v) { return v.as.obj; }
inline ObjString *asString(const Value &v) { return (ObjString *)v.as.obj; }

// Print a value to stdout. Strings are printed without quotes.
void printValue(const Value &value);

// Test for equality between two values. Only values of the same
// type can be equal. Numbers and booleans compare by value, nil
// compares to nil, and objects compare by pointer (since strings
// are interned, pointer equality means string equality).
bool valuesEqual(const Value &a, const Value &b);