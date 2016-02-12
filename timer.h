#ifndef __TIMER_H__
#define __TIMER_H__

#include <sys/types.h>
#include <sys/time.h>

typedef struct _timer
{
	struct timeval tv_start;
	struct timeval tv_end;
} timer;

void timer_start(timer *tm);
void timer_stop(timer *tm);
void timer_elapsed(timer *tm, struct timeval *tv);
void timer_lap(timer *tm, struct timeval *tv);
void timer_print_elapsed(timer *tm);
void timer_print_lap(timer *tm);
void timer_print_timeval(struct timeval *tv);

#endif // __TIMER_H__
