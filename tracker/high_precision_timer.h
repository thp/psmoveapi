#ifndef HIGH_PRECISION_TIMER_H_DEF
#define HIGH_PRECISION_TIMER_H_DEF

#ifdef WIN32   // Windows system specific
#include <windows.h>
#else          // Unix based system specific
#include <sys/time.h>
#endif

/* Opaque data type for the PS Move internal data */
struct _HPTimer;
typedef struct _HPTimer HPTimer;

HPTimer* hp_timer_create(); // constructor, creates internal data structures of the timer
void hp_timer_release(HPTimer* t); // start timer
void hp_timer_start(HPTimer* t); // start timer
void hp_timer_stop(HPTimer* t); // stop the timer
double hp_timer_get_seconds(HPTimer* t); // get elapsed time in second
double hp_timer_get_millis(HPTimer* t); // get elapsed time in milli-second
double hp_timer_get_micros(HPTimer* t); // get elapsed time in micro-second

#endif // HIGH_PRECISION_TIMER_H_DEF
