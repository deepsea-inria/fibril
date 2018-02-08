#include <sys/time.h>

#ifndef LOG_H
#define LOG_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

typedef struct {
  char __padding1[128];
  int nb_steals;
  size_t time_stealing;
  char __padding2[128];
} log_t;

extern log_t* _logs;

extern size_t time_start;

size_t static inline time_since(size_t val)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec - val;
}

#define LOG_INIT(id) \
  _logs[id].nb_steals = 0;

#define LOG_COUNT_STEAL(id) \
  _logs[id].nb_steals++; \

#define LOG_START_STEALING(id) \
  time_since(0); \

#define LOG_END_STEALING(id, start) \
  _logs[id].time_stealing += time_since(start); \

#define LOG_EMIT(nprocs) \
  do { \
    int nb_steals = 0; \
    for (int i = 0; i < nprocs; i++) { \
      nb_steals += _logs[i].nb_steals; \
    } \
    printf("nb_steals\t%d\n", nb_steals); \
  } while (0); \

#endif /* end of include guard: LOG_H */
