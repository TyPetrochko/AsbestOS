#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/spinlock.h>
#include <lib/debug.h>
#include <dev/lapic.h>
#include <pcpu/PCPUIntro/export.h>
#include <kern/thread/PTCBIntro/export.h>

#include "import.h"

static spinlock_t sched_lk;

unsigned int sched_ticks[NUM_CPUS];

void thread_init(unsigned int mbi_addr)
{
  unsigned int i;
  for(i = 0; i < NUM_CPUS; i++) {
    sched_ticks[i] = 0;
  }

  spinlock_init(&sched_lk);
  tqueue_init(mbi_addr);
  set_curid(0);
  tcb_set_state(0, TSTATE_RUN);
}

/**
 * Allocates new child thread context, set the state of the new child thread
 * as ready, and pushes it to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
  unsigned int pid;

  spinlock_acquire(&sched_lk);

  pid = kctx_new(entry, id, quota);
  tcb_set_state(pid, TSTATE_READY);
  tcb_set_cpu(pid, get_pcpu_idx());
  tqueue_enqueue(NUM_IDS + get_pcpu_idx(), pid);
	
  spinlock_release(&sched_lk);
  
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
  unsigned int old_cur_pid;
  unsigned int new_cur_pid;

  spinlock_acquire(&sched_lk);

  old_cur_pid = get_curid();
  tcb_set_state(old_cur_pid, TSTATE_READY);
  tqueue_enqueue(NUM_IDS + get_pcpu_idx(), old_cur_pid);

  new_cur_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
  tcb_set_state(new_cur_pid, TSTATE_RUN);
  set_curid(new_cur_pid);

  if (old_cur_pid != new_cur_pid){
    spinlock_release(&sched_lk);
    kctx_switch(old_cur_pid, new_cur_pid);
  }
  else {
    spinlock_release(&sched_lk);
  }
}

void sched_update(void)
{
  spinlock_acquire(&sched_lk);
  sched_ticks[get_pcpu_idx()] += (1000 / LAPIC_TIMER_INTR_FREQ);
  if (sched_ticks[get_pcpu_idx()] > SCHED_SLICE) {
    sched_ticks[get_pcpu_idx()] = 0;
    spinlock_release(&sched_lk);
    thread_yield();
  }
  else{
    spinlock_release(&sched_lk);
  }
}


/**
 * Atomically release lock and sleep on chan.
 * Reacquires lock when awakened.
 */
void thread_sleep (void *chan, spinlock_t *lk)
{
  //TODO: your local variables here.
  unsigned int pid = get_curid();
  unsigned int new_pid;

  if (lk == 0)
    KERN_PANIC("sleep without lock");

  // Must acquire sched_lk in order to
  // change the current thread's state and then switch.
  // Once we hold sched_lk, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with sched_lk locked),
  // so it's okay to release lock.
  spinlock_acquire(&sched_lk);
  spinlock_release(lk);

  // Go to sleep.
  tcb_set_state(pid, TSTATE_SLEEP);
  tcb_set_chan(pid, chan);

  // Context switch.
  new_pid = tqueue_dequeue(NUM_IDS + get_pcpu_idx());
  if(new_pid == NUM_IDS)
    KERN_PANIC("sleep without another thread to switch to");
  set_curid(new_pid);
  tcb_set_state(new_pid, TSTATE_RUN);

  spinlock_release(&sched_lk);
  kctx_switch(pid, new_pid);

  // Tidy up.
  spinlock_acquire(&sched_lk);
  tcb_set_chan(pid, 0);
  // Reacquire original lock.

  //Is this the correct order?
  spinlock_acquire(lk);
  spinlock_release(&sched_lk);
}

/**
 * Wake up all processes sleeping on chan.
 */
void thread_wakeup (void *chan)
{
  unsigned int i;
  void * current_chan;
  unsigned int state;

  spinlock_acquire(&sched_lk);

  for (i = 0; i < NUM_IDS; i++) {
    state = tcb_get_state(i);
    current_chan = tcb_get_chan(i);
    if (state == TSTATE_SLEEP && current_chan == chan) {
      tcb_set_state(i, TSTATE_READY);
      tcb_set_chan(i, 0);
      tqueue_enqueue(NUM_IDS + tcb_get_cpu(i), i);
    }
  }

  spinlock_release(&sched_lk);
}
