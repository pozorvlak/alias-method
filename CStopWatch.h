#ifndef _CSTOPWATCH_H_
#define _CSTOPWATCH_H_

#include <stdio.h>
#include <sys/time.h>

#ifndef INLINE
#define INLINE inline __attribute__((always_inline))
#endif

#define SWTICK(NAME) _Pragma("omp master") __tick(&sw ## NAME)
#define SWTOCK(NAME) _Pragma("omp master") __tock(&sw ## NAME)
#define SWRESET(NAME) _Pragma("omp master") __reset(&sw ## NAME)
#define SWGET(NAME) __get(&sw ## NAME)
#define DEFSW(NAME) Stopwatch sw ## NAME; SWRESET(NAME)

typedef struct {
  unsigned long totaltime;
  struct timeval start;
  struct timeval stop;
} Stopwatch;

INLINE static void __reset(Stopwatch * S) {

  S->totaltime = 0;

}

INLINE static void __tick(Stopwatch * S) {

  gettimeofday(&S->start, NULL);

}

INLINE static void __tock(Stopwatch * S) {

  gettimeofday(&S->stop, NULL);
  S->totaltime += (S->stop.tv_sec - S->start.tv_sec) * 1000000 + (S->stop.tv_usec - S->start.tv_usec);

}

INLINE static unsigned long __get(Stopwatch *S) {

  return S->totaltime;

}

#endif
