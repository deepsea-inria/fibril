#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include "safe.h"
#include "debug.h"
#include "param.h"
#include "stats.h"
#include "log.h"

static pthread_t * _procs;
static void ** _stacks;

__thread int _tid;

log_t* _logs;
size_t time_start;

void fibril_rt_log_init();
void fibril_log_emit();

extern void fibrili_init(int id, int nprocs);
extern void fibrili_exit(int id, int nprocs);

#ifdef FIBRIL_STATS
void * MAIN_STACK_TOP;
#endif

static void * __main(void * id)
{
  _tid = (int) (intptr_t) id;
  
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

  size_t stacksize = PARAM_STACK_SIZE;

  _procs = malloc(sizeof(pthread_t [nprocs]));
  _stacks = malloc(sizeof(void * [nprocs]));
  _logs = malloc(sizeof(log_t [nprocs]));

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

void fibril_rt_log_stats_reset() {
  int nprocs = PARAM_NPROCS;
  for (int i = 0; i < nprocs; i++) {
    _logs[i].nb_steals = 0;
    _logs[i].time_stealing = 0;
  }
  time_start = fibril_time_since(0);
}

void fibril_rt_log_init() {
  fibril_rt_log_stats_reset();
}

void fibril_log_emit() {
  int nprocs = PARAM_NPROCS;
  {
    int nb_steals = 0;
    for (int i = 0; i < nprocs; ++i) {
      nb_steals += _logs[i].nb_steals;
    }
    printf("nb_steals\t%d\n", nb_steals);
  }
  {
    size_t total_idle_time_usec = 0;
    for (int i = 0; i < nprocs; ++i) {
      total_idle_time_usec += _logs[i].time_stealing;
    }
    double total_idle_time_sec = total_idle_time_usec / 1000000.0;
    printf("total_idle_time\t%f\n", total_idle_time_sec);
    double total_time_sec = fibril_time_since(time_start) / 1000000.0;
    printf("launch_duration\t%f\n", total_time_sec);
    double cumulative_time = total_time_sec * nprocs;
    double relative_idle = total_idle_time_sec / cumulative_time;
    double utilization = 1.0 - relative_idle;
    printf("utilization\t%f\n", utilization);
  }
}
