#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>
#include <lib/structures.h>

#include "import.h"

//1 lock for each queue
spinlock_t locks[NUM_CPUS];
unsigned int timeSinceSwitch[NUM_CPUS];

void thread_init(unsigned int mbi_addr)
{
	tqueue_init(mbi_addr);
	set_curid(0);
	tcb_set_state(0, TSTATE_RUN);
	//init all locks
	int i;
	for (i = 0; i < NUM_CPUS; i++) {
		spinlock_init(&locks[i]);
		timeSinceSwitch[i] = 0;
	}
}

/**
 * Allocates new child thread context, set the state of the new child thread
 * as ready, and pushes it to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
	int cpu_index = get_pcpu_idx();
	spinlock_acquire(&locks[cpu_index]);

	unsigned int pid;

	pid = kctx_new(entry, id, quota);
	tcb_set_cpu(pid, cpu_index);
	tcb_set_state(pid, TSTATE_READY);
	tqueue_enqueue(NUM_IDS + cpu_index, pid);

	spinlock_release(&locks[cpu_index]);
	return pid;
}

/**
 * Yield to the next thread in the ready queue.
 * You should set the currently running thread state as ready,
 * and push it back to the ready queue.
 * And set the state of the poped thread as running, set the
 * current thread id, then switches to the new kernel context.
 * Hint: if you are the only thread that is ready to run,
 * do you need to switch to yourself?
 */
void thread_yield(void)
{
	int cpu_index = get_pcpu_idx();
	spinlock_acquire(&locks[cpu_index]);
	unsigned int old_cur_pid;
	unsigned int new_cur_pid;

	old_cur_pid = get_curid();
	tcb_set_state(old_cur_pid, TSTATE_READY);
	tqueue_enqueue(NUM_IDS + cpu_index, old_cur_pid);

	new_cur_pid = tqueue_dequeue(NUM_IDS + cpu_index);
	tcb_set_state(new_cur_pid, TSTATE_RUN);
	set_curid(new_cur_pid);

	if (old_cur_pid != new_cur_pid){
		spinlock_release(&locks[cpu_index]);
		kctx_switch(old_cur_pid, new_cur_pid);
	} else {
		spinlock_release(&locks[cpu_index]);
	}
}

// indefinitely put a thread to sleep - thread_yield without readying
void thread_sleep(spinlock_t* sl){
  // PSEUDOCODE:
  //  acquire cpu spinlock
  //  set my tcb state to TSTATE_SLEEP
  //  get next ready thread from current CPU's ready queue
  //  set ^ TCB state of next ready thread to TSTATE_RUN, set cur_id, etc
  //    (see PThread.c, basically mimicing thread_yield)
  //  release cpu spinlock
  //  release SL
  //  switch to new thread
  
	int cpu_index = get_pcpu_idx();
	spinlock_acquire(&locks[cpu_index]);
	unsigned int old_cur_pid;
	unsigned int new_cur_pid;

	old_cur_pid = get_curid();
	tcb_set_state(old_cur_pid, TSTATE_SLEEP);

	new_cur_pid = tqueue_dequeue(NUM_IDS + cpu_index);
	tcb_set_state(new_cur_pid, TSTATE_RUN);
	set_curid(new_cur_pid);

  spinlock_release(&locks[cpu_index]);
  spinlock_release(sl);
  kctx_switch(old_cur_pid, new_cur_pid);
}

// wake up a sleeping thread
void thread_wake(unsigned int pid){
    // PSEUDOCODE (pid):
    //  cpu = tcb_get_cpu(pid)
    //  acquire cpu's spinlock
    //  set next_pid's TCB status to TSTATE_READY
    //  enqueue next_pid on cpu's ready list
    //  release cpu's spinlock
	
  int cpu_index = tcb_get_cpu(pid);
	spinlock_acquire(&locks[cpu_index]);
	
	tcb_set_state(pid, TSTATE_READY);
	tqueue_enqueue(NUM_IDS + cpu_index, old_cur_pid);

  spinlock_release(&locks[cpu_index]);
}

//update the time since switch for a given cpu
//we dont need locking since every cpu only updates its own array indece? I think
void sched_update(void) {
	int cpu_index = get_pcpu_idx();
	timeSinceSwitch[cpu_index] += 1000 / LAPIC_TIMER_INTR_FREQ;
	//check whether we need to switch
	if (timeSinceSwitch[cpu_index] >= SCHED_SLICE) {
		timeSinceSwitch[cpu_index] = 0;
		thread_yield();
	}
	return;
}

