#pragma once

#include <cassert>
#include <cwchar>
#include <stdlib.h>
#include <windows.h>

#define STRINGIFY2_(x) #x
#define STRINGIFY(x) STRINGIFY2_(x)

#pragma region Assert Macros
#ifdef _DEBUG
#define ASSERT_IMPL(x, format, ...) \
    auto result = Assert::Logger(x, format, __VA_ARGS__); \
    if (result == IDABORT) { \
        Assert::Abort(); \
    } \
    else if (result == IDRETRY)  { \
        __debugbreak(); \
    }

#define ASSERT(x) \
    do { \
        if (!(x)) { \
            ASSERT_IMPL(L"Assert Failed: "##STRINGIFY(x), L""); \
        } \
    } while(0)

#define ASSERT_MSG(x, format, ...) \
    do { \
        if (!(x)) { \
            ASSERT_IMPL(L"Assert Failed: "##STRINGIFY(x), format, __VA_ARGS__); \
        } \
    } while(0)

#define ERROR_MSG(format, ...) \
    do { \
        ASSERT_IMPL(L"", format, __VA_ARGS__); \
    } while(0)

#else
#define ASSERT(x)
#define ASSERT_MSG(x, format, ...)
#define ERROR_MSG(format, ...)
#endif
#pragma endregion

#pragma region Primitive Types
typedef char i8;
typedef short i16;
typedef long i32;
typedef long long i64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned long u32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;
typedef char char8;
typedef wchar_t char16;
#pragma endregion

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

namespace Assert {

static int Logger(wchar_t const *msg, wchar_t const *format, ...) {
    wchar_t buffer[1024];

    auto count = swprintf_s(buffer, countof(buffer), msg);

    va_list varg;
    va_start(varg, format);

    if (count >= 0) {
        if (count > 0) {
            buffer[count++] = L'\n';
        }
        vswprintf_s(buffer + count, countof(buffer) - count, format, varg);
    }
    else {
        vswprintf_s(buffer, countof(buffer), format, varg);
    }

    va_end(varg);

    return MessageBox(NULL, buffer, L"Assertion Triggered", MB_ABORTRETRYIGNORE);
}

static void Abort() {
    exit(IDABORT);
}

}
