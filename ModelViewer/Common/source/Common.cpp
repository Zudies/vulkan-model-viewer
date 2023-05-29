#include "pch.h"

#define __STDC_WANT_LIB_EXT1__ 1
#include <time.h>

namespace Assert {

int Logger(wchar_t const *msg, wchar_t const *format, ...) {
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

void Abort() {
    exit(IDABORT);
}

} // namespace Assert

namespace Logger {

void LogConsoleImpl(FILE *stream, char const *format, va_list args) {
    time_t ltime = time(NULL);
    struct tm result;

#if defined(_MSC_VER)
    localtime_s(&result, &ltime);
#elif defined(__STDC_LIB_EXT1__)
    localtime_s(&ltime, &result);
#else
    memcpy(&result, localtime(&ltime), sizeof(struct tm));
#endif

    fprintf(stream, "%02u:%02u:%02u ", result.tm_hour, result.tm_min, result.tm_sec);
    vfprintf_s(stream, format, args);
    fflush(stream);
}

void LogConsoleImpl(FILE *stream, wchar_t const *format, va_list args) {
    time_t ltime = time(NULL);
    struct tm result;

#if defined(_MSC_VER)
    localtime_s(&result, &ltime);
#elif defined(__STDC_LIB_EXT1__)
    localtime_s(&ltime, &result);
#else
    memcpy(&result, localtime(&ltime), sizeof(struct tm));
#endif

    fwprintf(stream, L"%02u:%02u:%02u ", result.tm_hour, result.tm_min, result.tm_sec);
    vfwprintf_s(stream, format, args);
    fflush(stream);
}

void LogConsole(char const *format, ...) {
    va_list varg;
    va_start(varg, format);
    LogConsoleImpl(stdout, format, varg);
    va_end(varg);
}

void LogConsole(wchar_t const *format, ...) {
    va_list varg;
    va_start(varg, format);
    LogConsoleImpl(stdout, format, varg);
    va_end(varg);
}

void ErrorConsole(char const *format, ...) {
    va_list varg;
    va_start(varg, format);
    LogConsoleImpl(stderr, format, varg);
    va_end(varg);
}

void ErrorConsole(wchar_t const *format, ...) {
    va_list varg;
    va_start(varg, format);
    LogConsoleImpl(stderr, format, varg);
    va_end(varg);
}

} // namespace Logger
