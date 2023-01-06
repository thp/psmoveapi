#pragma once

#include <cstdio>
#include <cstdarg>

#include <string>

namespace {

inline std::string format(const char *fmt, ...)
#if !defined(_MSC_VER)
    __attribute__ ((format (printf, 1, 2)))
#endif
;

inline std::string
format(const char *fmt, ...)
{
    std::string result;

    va_list ap;
    va_start(ap, fmt);

    char *tmp = 0;
#if defined(_WIN32)
    size_t len = 4096;
    tmp = (char *)malloc(len);
    int res = vsnprintf(tmp, len, fmt, ap);
    PSMOVE_VERIFY(res != -1, "vsnprintf() failed");
#else
    int res = vasprintf(&tmp, fmt, ap);
    PSMOVE_VERIFY(res != -1, "vasprintf() failed");
#endif

    va_end(ap);

    result = tmp;
    free(tmp);

    return result;
}

} // end anonymous namespace
