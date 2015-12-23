#include "unistd.h"

/* usleep for MSVC taken from http://stackoverflow.com/a/17283549/1256069 */
#ifdef _MSC_VER
#include <windows.h>
int usleep(__int64 useconds)
{
    HANDLE timer;
    LARGE_INTEGER ft;

    ft.QuadPart = -(10*useconds); // Convert to 100 nanosecond interval, negative value indicates relative time

    timer = CreateWaitableTimer(NULL, TRUE, NULL);
    SetWaitableTimer(timer, &ft, 0, NULL, NULL, 0);
    WaitForSingleObject(timer, INFINITE);
    CloseHandle(timer);
    return 0;
};
#endif