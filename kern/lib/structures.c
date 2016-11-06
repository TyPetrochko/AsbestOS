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
  return ret;
}

int queue_empty(queue *q){
  return (q->head == q->tail);
}

// UTIL -- LOCKS
int lock_ctr = 0;

void lock_init(Lock *lock){
  spinlock_init(&(lock->spinlock));
}

void lock_acquire(Lock *lock){
  spinlock_acquire(&(lock->spinlock));
}

void lock_release(Lock *lock){
  spinlock_release(&(lock->spinlock));
}

//UTIL -- CV

void cv_init(CV *cond) {
  queue_init(&(cond->waiting));
  spinlock_init(&(cond->spinlock));
}

void cv_wait(CV *cond, Lock *lock) {
  unsigned int pid;
  intr_local_disable();

  pid = get_curid();
  //put pid on cv's queue
  enqueue(&(cond->waiting), pid);
  //sleep on variable and release lock
  lock_release(lock);
  spinlock_acquire(&(cond->spinlock));
  if(cond->ticket != pid)
  while(cond->ticket != pid){
    spinlock_release(&(cond->spinlock));
    spinlock_acquire(&(cond->spinlock));
  }
  cond->ticket = NUM_IDS; // reset ticket
  lock_acquire(lock);
  spinlock_release(&(cond->spinlock));
  intr_local_enable();
}

void cv_signal(CV *cond) {
  intr_local_disable();
  if (!queue_empty(&(cond->waiting))) {
  	unsigned int pid = dequeue(&(cond->waiting));
    spinlock_acquire(&(cond->spinlock));
    cond->ticket = pid;
    spinlock_release(&(cond->spinlock));
  }
  intr_local_enable();
}

//UTIL -- Bounded Buffer

void buffer_init(BB *buffer) {
	//initialization
	buffer->head = 0;
	//tail is upper bound, all values are between head inclusive and tail non-inclusive
	buffer->tail = 0;
	lock_init(&(buffer->lock));
	cv_init(&(buffer->empty));
	cv_init(&(buffer->full));
}

void buffer_put(int data, BB *buffer) {
	unsigned int pid = get_curid();
	lock_acquire(&(buffer->lock));

	//use while loop for wait
	//while full
	while ((buffer->tail - buffer->head) == BUFFER_SIZE) {
		cv_wait(&(buffer->full), &(buffer->lock));
	}

	//update vals
	buffer->buffer[buffer->tail % BUFFER_SIZE] = data;
	buffer->tail++;
  intr_local_disable();
  KERN_DEBUG("Process %u produced %d.\n", pid, data);
  intr_local_enable();
	//signal anyone waiting on empty
	cv_signal(&(buffer->empty));
	lock_release(&(buffer->lock));
}

int buffer_get(BB *buffer) {
	unsigned int pid = get_curid();
	lock_acquire(&(buffer->lock));

	//wait for not empty
	while (buffer->head == buffer->tail) {
		cv_wait(&(buffer->empty), &(buffer->lock));
	}

	//update vals
	int retval = buffer->buffer[buffer->head % BUFFER_SIZE];
  intr_local_disable();
  KERN_DEBUG("Process %u consumed %d.\n", pid, retval);
  intr_local_enable();
	buffer->head++;

	//signal to anyone waiting on full
	cv_signal(&(buffer->full));
	lock_release(&(buffer->lock));
  return retval;
}

