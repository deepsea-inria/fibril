#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <stdio.h>
#include "safe.h"
#include "debug.h"
#include "param.h"
#include "stats.h"
#include "log.h"

#ifdef FIBRIL_USE_HWLOC
#include <hwloc.h>
hwloc_topology_t topology;
#endif

static pthread_t * _procs;
static void ** _stacks;

__thread int _tid;

log_t* _logs;
size_t log_stats_time_start;
size_t log_time_start;

void fibril_rt_log_init();
void fibril_log_emit();

#if defined(FIBRIL_USE_HWLOC)

void fibril_hwloc_init() {
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
}

void fibril_hwloc_worker_init(int id) {
  // bind worker thread with identifier id to core number id
  int nb_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
  if (id >= nb_cores) {
    return;
  }
  int flags = HWLOC_CPUBIND_STRICT | HWLOC_CPUBIND_THREAD;
  int core_depth = hwloc_get_type_or_below_depth(topology, HWLOC_OBJ_CORE);
  hwloc_obj_t core_obj = hwloc_get_obj_by_depth(topology, core_depth, id);
  hwloc_cpuset_t cpuset = hwloc_bitmap_dup(core_obj->cpuset);
  if (hwloc_set_cpubind(topology, cpuset, flags)) {
    char *str;
    int error = errno;
    hwloc_bitmap_asprintf (&str, cpuset);
    printf ("Couldn't bind to cpuset %s: %s\n", str, strerror(error));
    free (str);
    return;
  }
  free(cpuset);
}

#else

void fibril_hwloc_init() { }
void fibril_hwloc_worker_init(int id) { }

#endif

extern void fibrili_init(int id, int nprocs);
extern void fibrili_exit(int id, int nprocs);

#ifdef FIBRIL_STATS
void * MAIN_STACK_TOP;
#endif

static void * __main(void * id)
{
  _tid = (int) (intptr_t) id;
  
  fibril_hwloc_worker_init(_tid);
  
  fibrili_init(_tid, PARAM_NPROCS);
  return NULL;
}

int fibril_rt_nprocs()
{
  if (PARAM_NPROCS == 0) {
    return param_nprocs(0);
  } else {
    return PARAM_NPROCS;
  }
}

int fibril_rt_init(int n)
{
  param_init(n);

  int nprocs = PARAM_NPROCS;
  if (nprocs <= 0) return -1;

  fibril_hwloc_init();

  size_t stacksize = PARAM_STACK_SIZE;

  _procs = malloc(sizeof(pthread_t [nprocs]));
  _stacks = malloc(sizeof(void * [nprocs]));

  fibril_rt_log_init();

  pthread_attr_t attrs[nprocs];
  int i;

  for (i = 1; i < nprocs; ++i) {
    SAFE_RZCALL(posix_memalign(&_stacks[i], PARAM_PAGE_SIZE, stacksize));
    pthread_attr_init(&attrs[i]);
    pthread_attr_setstack(&attrs[i], _stacks[i], stacksize);
    pthread_create(&_procs[i], &attrs[i], __main, (void *) (intptr_t) i);
    pthread_attr_destroy(&attrs[i]);
  }

  _procs[0] = pthread_self();
  SAFE_RZCALL(posix_memalign(&_stacks[0], PARAM_PAGE_SIZE, stacksize));

  register void * rsp asm ("r15");
  rsp = _stacks[0] + stacksize;

#ifdef FIBRIL_STATS
  register void * top asm ("rsp");
  MAIN_STACK_TOP = PAGE_ALIGN_DOWN(top);
#endif

  __asm__ ( "xchg\t%0,%%rsp" : "+r" (rsp) :: "memory" );
  __main((void *) 0);
  __asm__ ( "xchg\t%0,%%rsp" : : "r" (rsp) : "memory" );

  return 0;
}

int fibril_rt_exit()
{
  fibrili_exit(_tid, PARAM_NPROCS);

  int i;

  for (i = 1; i < PARAM_NPROCS; ++i) {
    pthread_join(_procs[i], NULL);
    free(_stacks[i]);
  }

  fibril_log_emit(PARAM_NPROCS);

  free(_procs);
  free(_stacks);
  free(_logs);

  STATS_EXPORT(N_STEALS);
  STATS_EXPORT(N_SUSPENSIONS);
  STATS_EXPORT(N_STACKS);
  STATS_EXPORT(N_PAGES);
    
  return 0;
}

#if defined(FIBRIL_LOG)

void fibril_rt_log_stats_init() {
  int nprocs = PARAM_NPROCS;
  for (int i = 0; i < nprocs; i++) {
    _logs[i].nb_steals = 0;
    _logs[i].time_stealing = 0;
    
  }
  log_stats_time_start = fibril_time_since(0);
}

void fibril_rt_log_stats_reset() {
  fibril_rt_log_stats_init();
  LOG_PUSH_EVENT(0, enter_algo);
}

void fibril_rt_log_init() {
  int nprocs = PARAM_NPROCS;
  _logs = malloc(sizeof(log_t [nprocs]));
  for (int i = 0; i < nprocs; i++) {
    _logs[i].events = create_block(NULL);
  }
  log_time_start = fibril_time_since(0);
  LOG_PUSH_EVENT(0, enter_launch);
  fibril_rt_log_stats_init();
}

int compare_log_events(const void* a, const void* b) {
  const log_event_t* ea = (const log_event_t*)a;
  const log_event_t* eb = (const log_event_t*)b;
  return (ea->timestamp > eb->timestamp) - (ea->timestamp < eb->timestamp);
}

void fibril_log_emit() {
  int nprocs = PARAM_NPROCS;
  { // report nb steals
    int nb_steals = 0;
    for (int i = 0; i < nprocs; ++i) {
      nb_steals += _logs[i].nb_steals;
    }
    printf("nb_steals\t%d\n", nb_steals);
  }
  { // report total idle time, launch duration, and utilization
    size_t total_idle_time_usec = 0;
    for (int i = 0; i < nprocs; ++i) {
      total_idle_time_usec += _logs[i].time_stealing;
    }
    double total_idle_time_sec = total_idle_time_usec / 1000000.0;
    printf("total_idle_time\t%f\n", total_idle_time_sec);
    double total_time_sec = fibril_time_since(log_stats_time_start) / 1000000.0;
    printf("launch_duration\t%f\n", total_time_sec);
    double cumulative_time = total_time_sec * nprocs;
    double relative_idle = total_idle_time_sec / cumulative_time;
    double utilization = 1.0 - relative_idle;
    printf("utilization\t%f\n", utilization);
  }
  { // emit logging data
    LOG_PUSH_EVENT(0, exit_algo);
    LOG_PUSH_EVENT(0, exit_launch);
    size_t total_nb_events = 0;
    for (int i = 0; i < nprocs; ++i) {
      log_block_t* block = _logs[i].events;
      while (block != NULL) {
        total_nb_events += block->nb_events;
        block = block->tl;
      }
    }
    log_event_t* events = malloc(sizeof(log_event_t [total_nb_events]));
    size_t k = 0;
    for (int i = 0; i < nprocs; ++i) {
      log_block_t* block = _logs[i].events;
      _logs[i].events = NULL;
      while (block != NULL) {
        int nb_events = block->nb_events;
        memcpy(&events[k], &(block->hd[0]), sizeof(log_event_t) * nb_events);
        k += nb_events;
        log_block_t* tl = block->tl;
        free(block);
        block = tl;
      }
    }
    qsort(events, total_nb_events, sizeof(log_event_t), compare_log_events);
    FILE* fp;
    fp = fopen("LOG_BIN", "w");
    for (size_t i = 0; i < total_nb_events; ++i) {
      int64_t t = (int64_t)events[i].timestamp;
      fwrite(&t, sizeof(int64_t), 1, fp);
      t = (int64_t)events[i].worker_id;
      fwrite(&t, sizeof(int64_t), 1, fp);
      t = (int64_t)events[i].tag;
      fwrite(&t, sizeof(int64_t), 1, fp);
    }
    fclose(fp);
    free(events);
  }
}

#else

void fibril_rt_log_init() { }
void fibril_rt_log_stats_reset() { }
void fibril_log_emit() { }

#endif
