#ifndef SCHED_H
#define SCHED_H

#include "util.h"
#include "stack.h"
#include "fibril.h"
#include "fibrili.h"

__attribute__ ((noreturn))
void sched_work(int id, int procs);
void sched_exit();

static inline __attribute__ ((noreturn))
void sched_restart()
{
  int id = fibrile_deq.tid;
  STACK_EXECUTE(_stacks[id], sched_work(id, _nprocs));
}

static inline __attribute__ ((noreturn))
void sched_resume(const fibril_t * frptr)
{
  __asm__ (
      "mov\t%0,%%rsp\n\t"
      "mov\t%1,%%rbp\n\t"
      "mov\t%2,%%rbx\n\t"
      "mov\t%3,%%r12\n\t"
      "mov\t%4,%%r13\n\t"
      "mov\t%5,%%r14\n\t"
      "mov\t%6,%%r15\n\t"
      "jmp\t*(%7)" : :
      "g" (frptr->rsp), "g" (frptr->rbp),
      "g" (frptr->rbx), "g" (frptr->r12),
      "g" (frptr->r13), "g" (frptr->r14),
      "g" (frptr->r15), "g" (&frptr->rip)
  );

  unreachable();
}

#endif /* end of include guard: SCHED_H */