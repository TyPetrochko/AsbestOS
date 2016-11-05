#ifndef _KERN_THREAD_PTHREAD_H_
#define _KERN_THREAD_PTHREAD_H_

#ifdef _KERN_

#include <lib/spinlock.h>
#include <lib/structures.h>

void thread_init(unsigned int mbi_addr);
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota);
void thread_yield(void);
void thread_update(void);
void sched_update(void);
void thread_sleep(spinlock_t* sl);

//copy of thread sleep but using a regular lock
void thread_sleep_with_lock(Lock *lock);

// wake up a sleeping thread
void thread_wake(unsigned int pid);

#endif /* _KERN_ */

#endif /* !_KERN_THREAD_PTHREAD_H_ */

