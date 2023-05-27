// pch.cpp: source file corresponding to the pre-compiled header

#include "pch.h"

// When you are using pre-compiled headers, this source file is necessary for compilation to succeed.
// Sanity check basic types
static_assert(sizeof(int8) == 1, "Primitive type size error");
static_assert(sizeof(int16) == 2, "Primitive type size error");
static_assert(sizeof(int32) == 4, "Primitive type size error");
static_assert(sizeof(int64) == 8, "Primitive type size error");
static_assert(sizeof(uint8) == 1, "Primitive type size error");
static_assert(sizeof(uint16) == 2, "Primitive type size error");
static_assert(sizeof(uint32) == 4, "Primitive type size error");
static_assert(sizeof(uint64) == 8, "Primitive type size error");
static_assert(sizeof(float32) == 4, "Primitive type size error");
static_assert(sizeof(float64) == 8, "Primitive type size error");
static_assert(sizeof(char8) == 1, "Primitive type size error");
static_assert(sizeof(char16) == 2, "Primitive type size error");
