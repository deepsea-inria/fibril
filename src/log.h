#include <string.h>
#include <sys/time.h>

#ifndef LOG_H
#define LOG_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

typedef enum {
  enter_launch = 0,   exit_launch,
  enter_algo,         exit_algo,
  enter_wait,         exit_wait,
  worker_communicate, interrupt,
  algo_phase,
  nb_events
} log_event_tag_t;

typedef struct {
  double timestamp;
  int worker_id;
  log_event_tag_t tag;
} log_event_t;

#define BLOCK_CAPACITY 1024

typedef struct log_block_s {
  log_event_t hd[BLOCK_CAPACITY];
  int nb_events;
  struct log_block_s* tl;
} log_block_t;

typedef struct {
  char __padding1[128];
  int nb_steals;
  size_t time_stealing;
  log_block_t* events;
  size_t start_stealing;
  char __padding2[128];
} log_t;

extern log_t* _logs;

extern size_t log_time_start;

size_t static inline fibril_time_since(size_t val)
{
  struct timeval t;
  gettimeofday(&t, NULL);
  return t.tv_sec * 1000000 + t.tv_usec - val;
}

extern void fibril_rt_log_stats_reset();

static inline
const char* string_of_event_tag(log_event_tag_t t) {
  switch (t) {
  case enter_launch: return "enter_launch";
  case exit_launch: return "exit_launch";
  case enter_algo: return "enter_algo";
  case exit_algo: return "exit_algo";
  case enter_wait: return "enter_wait";
  case exit_wait: return "exit_wait";
  case algo_phase: return "algo_phase";
  default: return "bogus";
  }
}

void static inline print_event(log_event_t e) {
  printf("%f\t%d\t%s\n", e.timestamp, e.worker_id, string_of_event_tag(e.tag));
}

static inline
log_block_t* create_block(log_block_t* tl) {
  log_block_t* b = malloc(sizeof(log_block_t [1]));
  b->nb_events = 0;
  b->tl = tl;
  return b;
}

void static inline fibril_log_push_event(int id, log_event_tag_t tag) {
  log_event_t e;
  e.timestamp = fibril_time_since(log_time_start);
  e.worker_id = id;
  e.tag = tag;
  log_t* log_i = &_logs[id];
  log_block_t* events = log_i->events;
  if (events->nb_events == BLOCK_CAPACITY) {
    events = create_block(events);
    log_i->events = events;
  }
  int i = events->nb_events++;
  events->hd[i] = e;
}

#if defined(FIBRIL_LOG)

#define LOG_COUNT_STEAL(id) \
  _logs[id].nb_steals++; \

#define LOG_START_STEALING(id) \
  _logs[id].start_stealing = fibril_time_since(0); \

#define LOG_END_STEALING(id) \
  _logs[id].time_stealing += fibril_time_since(_logs[id].start_stealing); \

#define LOG_PUSH_EVENT(id, tag) \
  fibril_log_push_event(id, tag); \

#else

#define LOG_COUNT_STEAL(...)
#define LOG_START_STEALING(...)
#define LOG_END_STEALING(...)
#define LOG_PUSH_EVENT(...)

#endif

#endif /* end of include guard: LOG_H */
