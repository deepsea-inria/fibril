#ifndef FIBRIL_SERIAL_H
#define FIBRIL_SERIAL_H

#define fibril
#define fibril_t __attribute__((unused)) int
#define fibril_init(fp)
#define fibril_join(fp)

#define fibril_fork_nrt(fp, fn, ag) (fn ag)
#define fibril_fork_wrt(fp, rtp, fn, ag) (*rtp = fn ag)

#define fibril_rt_init(n)
#define fibril_rt_exit()
#define fibril_rt_nprocs(n) (1)
#define fibril_rt_log_stats_reset()

#endif /* end of include guard: FIBRIL_SERIAL_H */
