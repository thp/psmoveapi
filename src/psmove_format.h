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
    int res = vasprintf(&tmp, fmt, ap);
    PSMOVE_VERIFY(res != -1, "vasprintf() failed");
    va_end(ap);

    result = tmp;
    free(tmp);

    return result;
}

} // end anonymous namespace
