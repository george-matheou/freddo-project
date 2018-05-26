#include "Timer.h"
#include <sys/time.h>
#include <time.h>


double gtod_micro()
{
     struct timeval t;
     gettimeofday(&t,0);
     return t.tv_usec * 0.000001  + t.tv_sec;
}

double count_cycles()
{
	//return clock();
	
	uint64_t a, d;
	__asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
	return (d<<32) | a;
}

uint64_t rdtsc() 
{
  uint64_t a, d;
  __asm__ volatile ("rdtsc" : "=a" (a), "=d" (d));
  return (d<<32) | a;
}
