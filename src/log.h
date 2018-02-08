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

size_t static inline fibril_time_since(size_t val)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec - val;
}

extern void fibril_rt_log_stats_reset();

#define LOG_COUNT_STEAL(id) \
  _logs[id].nb_steals++; \

#define LOG_START_STEALING(id) \
  fibril_time_since(0); \

#define LOG_END_STEALING(id, start) \
  _logs[id].time_stealing += fibril_time_since(start); \

#endif /* end of include guard: LOG_H */
