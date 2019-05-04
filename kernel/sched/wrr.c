/*
 * Weighted Round Robin (WRR) Class (SCHED_WRR)
 */

#include "sched.h"

/*
 * default timeslice is 100 msecs (used only for SCHED_WRR tasks).
 * Timeslices get refilled after they expire.
 *
 * (ref)include/linux/sched/rt.h 65
 */
#define WRR_TIMESLICE (100 * HZ / 1000)

const struct sched_class wrr_sched_class;

static inline bool task_is_wrr(struct task_struct *tsk)
{
    int policy = tsk->policy;

    if(policy == SCHED_WRR)
        return true;

    return false;
}

static void task_tick_wrr(struct rq *rq, struct task_struct *p, int queued)
{
/*
 * load balancing : 2000ms
 * refer to other schedulers about load balancing if materials exists
 * 
 * Make sure that it only works when more than one CPU is active
 * CPU hotplug
 * for_each_online_cpu(cpu)
 * 
 * RQ_MIN
 * RQ_MAX
 *
 * pick a task(largest weight/not running/moving to RQ_MIN is possible)
 *
 */
}
const struct sched_class wrr_sched_class = {
    .next = &fair_sched_class,
    .task_tick = task_tick_wrr,
    .set_cpus_allowed = set_cpus_allowed_common,
};
