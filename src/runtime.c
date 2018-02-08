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

  for (int i = 0; i < nprocs; ++i) {
    LOG_INIT(i);
  }

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

  free(_procs);
  free(_stacks);
  free(_logs);

  STATS_EXPORT(N_STEALS);
  STATS_EXPORT(N_SUSPENSIONS);
  STATS_EXPORT(N_STACKS);
  STATS_EXPORT(N_PAGES);

  LOG_EMIT(PARAM_NPROCS);
    
  return 0;
}

