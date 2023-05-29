#pragma once

#include <cassert>
#include <cwchar>
#include <stdlib.h>
#include <windows.h>

#ifndef VERBOSE
#define VERBOSE 1
#endif

#define STRINGIFY2_(x) #x
#define STRINGIFY(x) STRINGIFY2_(x)

#pragma region Assert Macros
#ifdef _DEBUG
#define ASSERT_IMPL(x, format, ...) \
    auto _assert_result_ = Assert::Logger(x, format, __VA_ARGS__); \
    if (_assert_result_ == IDABORT) { \
        Assert::Abort(); \
    } \
    else if (_assert_result_ == IDRETRY)  { \
        __debugbreak(); \
    }

#define ASSERT(x) \
    do { \
        if (!(x)) { \
            ASSERT_IMPL(L"Assert Failed: " STRINGIFY(x), L""); \
        } \
    } while(0)

#define ASSERT_MSG(x, format, ...) \
    do { \
        if (!(x)) { \
            ASSERT_IMPL(L"Assert Failed: " STRINGIFY(x), format, __VA_ARGS__); \
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

#pragma region Logger Macros
#define LOG_INFO(format, ...) Logger::LogConsole(format, __VA_ARGS__)
#define LOG_ERROR(format, ...) Logger::LogError(format, __VA_ARGS__)

#if VERBOSE == 1
#define LOG_VERBOSE(format, ...) Logger::LogConsole(format, __VA_ARGS__)
#else
#define LOG_VERBOSE(format, ...)
#endif

#ifdef _DEBUG
#define LOG_DEBUG(format, ...) Logger::LogConsole(format, __VA_ARGS__)
#else
#define LOG_DEBUG(format, ...)
#endif
#pragma endregion

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

#define UNUSED_PARAM(x) (void)x

namespace Assert {

int Logger(wchar_t const *msg, wchar_t const *format, ...);

void Abort();

} // namespace Assert

namespace Logger {

void LogConsole(char const *format, ...);

void LogConsole(wchar_t const *format, ...);

void ErrorConsole(char const *format, ...);

void ErrorConsole(wchar_t const *format, ...);

} // namespace Logger
