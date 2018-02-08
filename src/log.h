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

typedef struct {
  char __padding1[128];
  int nb_steals;
  size_t time_stealing;
  log_event_t* events;
  size_t nb_events;
  size_t events_capacity;
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

#define LOG_COUNT_STEAL(id) \
  _logs[id].nb_steals++; \

#define LOG_START_STEALING(id) \
  _logs[id].start_stealing = fibril_time_since(0); \

#define LOG_END_STEALING(id) \
  _logs[id].time_stealing += fibril_time_since(_logs[id].start_stealing); \

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

void static inline fibril_log_push_event(int id, log_event_tag_t tag) {
  log_event_t e;
  e.timestamp = fibril_time_since(log_time_start);
  e.worker_id = id;
  e.tag = tag;
  log_t* log_i = &_logs[id];
  size_t old_nb_events = log_i->nb_events;
  size_t idx = old_nb_events;
  size_t new_nb_events = ++log_i->nb_events;
  size_t old_events_capacity = log_i->events_capacity;
  if (new_nb_events > old_events_capacity) {
    size_t new_events_capacity = old_events_capacity * 2;
    log_event_t* old_events = log_i->events;
    log_event_t* new_events = malloc(sizeof(log_event_t [new_events_capacity]));
    memcpy(new_events, old_events, sizeof(log_event_t) * old_nb_events);
    log_i->events = new_events;
    log_i->events_capacity = new_events_capacity;
    free(old_events);
  }
  log_event_t* events = log_i->events;
  events[idx] = e;
}

#define LOG_PUSH_EVENT(id, tag) \
  fibril_log_push_event(id, tag); \

#endif /* end of include guard: LOG_H */
