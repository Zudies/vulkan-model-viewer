// pch.cpp: source file corresponding to the pre-compiled header

#include "pch.h"

// When you are using pre-compiled headers, this source file is necessary for compilation to succeed.
// Sanity check basic types
static_assert(sizeof(i8) == 1, "Primitive type size error");
static_assert(sizeof(i16) == 2, "Primitive type size error");
static_assert(sizeof(i32) == 4, "Primitive type size error");
static_assert(sizeof(i64) == 8, "Primitive type size error");
static_assert(sizeof(u8) == 1, "Primitive type size error");
static_assert(sizeof(u16) == 2, "Primitive type size error");
static_assert(sizeof(u32) == 4, "Primitive type size error");
static_assert(sizeof(u64) == 8, "Primitive type size error");
static_assert(sizeof(f32) == 4, "Primitive type size error");
static_assert(sizeof(f64) == 8, "Primitive type size error");
static_assert(sizeof(char8) == 1, "Primitive type size error");
static_assert(sizeof(char16) == 2, "Primitive type size error");
