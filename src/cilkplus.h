#ifndef CILKPLUS_H
#define CILKPLUS_H

#include <cilk/cilk.h>

#define fibril
#define fibril_t __attribute__((unused)) int
#define fibril_init(fp)
#define fibril_join(fp) cilk_sync

#define fibril_fork_nrt(fp, fn, ag)     cilk_spawn fn ag
#define fibril_fork_wrt(fp, rt, fn, ag) *rt = cilk_spawn fn ag

#define fibril_rt_init(n) (__cilkrts_set_param("stack size", "0x800000"))
#define fibril_rt_exit() (__cilkrts_end_cilk())
#define fibril_rt_nprocs() (__cilkrts_get_nworkers())
#define fibril_rt_log_stats_reset()

#endif /* end of include guard: CILKPLUS_H */
