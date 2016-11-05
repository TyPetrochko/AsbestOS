#include <lib/debug.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/types.h>
#include <lib/trap.h>
#include <lib/syscall.h>
#include <dev/intr.h>
#include <pcpu/PCPUIntro/export.h>

#include "structures.h"

#define LOCK_FREE (1)
#define LOCK_BUSY (0)

// UTIL -- QUEUE
void queue_init(queue *q){
  q->head = 0;
  q->tail = 0;
}

void enqueue(queue *q, unsigned int pid){
  q->processes[q->tail] = pid;
  q->tail = q->tail + 1 % NUM_IDS;
}

unsigned int dequeue(queue *q){
  unsigned int ret = q->processes[q->head];
  q->head = q->head + 1 % NUM_IDS;
}

int queue_empty(queue *q){
  return (q->head == q->tail);
}

// UTIL -- LOCKS
void lock_init(Lock *lock){
  lock->free = LOCK_FREE;
  spinlock_init(&(lock->spinlock));
  queue_init(&(lock->waiting));
}

void lock_aquire(Lock *lock){
  unsigned int pid, cpu;

  // required for storing within lock
  pid = get_curid();
  cpu = get_pcpu_idx();

  intr_local_disable();
  spinlock_acquire(&(lock->spinlock));

  if(lock->free == LOCK_BUSY){
    // wait on this lock
    enqueue(&(lock->waiting), pid);
    // sleep indefinitely (wait for release to be called)
    thread_sleep(&(lock->spinlock));
    // acquire lock spinlock again since thread_sleep releases it
    spinlock_acquire(&(lock->spinlock));
  }

  // we've acquired the lock! It should not have been set to free in meantime
  if(lock->free != LOCK_BUSY)
    KERN_PANIC("Somehow lock became free while waiting on it.\n");

  spinlock_release(&(lock->spinlock));
  intr_local_enable();
}

void lock_release(Lock *lock){
  intr_local_disable();
  spinlock_acquire(&(lock->spinlock));

  if(!queue_empty(&(lock->waiting))){
    // wake up the next thread waiting on this lock
    thread_wake(dequeue(&(lock->waiting)));
  }else{
    // lock is ready to be picked up by anyone!
    lock->free = LOCK_FREE;
  }
  
  spinlock_release(&(lock->spinlock));
  intr_local_enable();
}

//UTIL -- CV

void cv_init(CV *cond) {
  queue_init(&(cond->waiting));
}

void cv_wait(CV *cond, unsigned int pid, Lock *lock) {
  intr_local_disable();
  //put pid on cv's queue
  enqueue(&(cond->waiting), pid);
  //sleep on variable and release lock
  thread_sleep_with_lock(lock);
  lock_aquire(lock);
  intr_local_enable();
}

void cv_signal(CV *cond) {
  intr_local_disable();
  if (queue_empty(&(cond->waiting))) {
  	unsigned int pid = dequeue(&(cond->waiting));
  	thread_wake(pid);
  }
  intr_local_enable();
}

//UTIL -- Bounded Buffer

//we only ever need one buffer, so declare it
Buffer b;

void buffer_init() {
	b.head = 0;
	//tail is upper bound, all values are between head inclusive and tail non-inclusive
	b.tail = 0;
	lock_init(&(b.lock));
	cv_init(&(b.empty));
	cv_init(&(b.full));
}

void buffer_put(int new) {
	lock_aquire(&(b.lock));
	unsigned int pid = get_curid();

	//use while loop for wait
	//while full
	while ((b.tail - b.head) == BUFFER_SIZE) {
		cv_wait(&(b.full), pid, &(b.lock));
	}

	//update vals
	b.buffer[b.tail % BUFFER_SIZE] = new;
	b.tail++;
	//signal anyone waiting on empty
	cv_signal(&(b.empty));
	lock_release(&(b.lock));
}

int buffer_get() {
	lock_aquire(&(b.lock));
	unsigned int pid = get_curid();

	//wait for not empty
	while (b.head == b.tail) {
		cv_wait(&(b.empty), pid, &(b.lock));
	}

	//update vals
	int retval = b.buffer[b.head % BUFFER_SIZE];
	b.head++;

	//signal to anyone waiting on full
	cv_signal(&(b.full));
	lock_release(&(b.lock));
}