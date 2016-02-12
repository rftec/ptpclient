#include "timer.h"
#include <stdlib.h>
#include <stdio.h>

void timer_start(timer *tm)
{
	tm->tv_end.tv_sec = 0;
	tm->tv_end.tv_usec = 0;
	gettimeofday(&tm->tv_start, NULL);
}

void timer_stop(timer *tm)
{
	gettimeofday(&tm->tv_end, NULL);
}

void timer_elapsed(timer *tm, struct timeval *tv)
{
	if (tm->tv_end.tv_sec == 0 && tm->tv_end.tv_usec == 0)
	{
		struct timeval tv_now;
		
		gettimeofday(&tv_now, NULL);
		timersub(&tv_now, &tm->tv_start, tv);
	}
	else
	{
		timersub(&tm->tv_end, &tm->tv_start, tv);
	}
}

void timer_lap(timer *tm, struct timeval *tv)
{
	timer_elapsed(tm, tv);
	timer_start(tm);
}

void timer_print_elapsed(timer *tm)
{
	struct timeval tv_dur;
	
	timer_elapsed(tm, &tv_dur);
	timer_print_timeval(&tv_dur);
}

void timer_print_lap(timer *tm)
{
	struct timeval tv_dur;
	
	timer_lap(tm, &tv_dur);
	timer_print_timeval(&tv_dur);
}

void timer_print_timeval(struct timeval *tv)
{
	printf(
		"%ld.%06ld seconds\n", 
		tv->tv_sec, 
		tv->tv_usec
	);
}
