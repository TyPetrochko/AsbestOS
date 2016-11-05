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

//#define say(...) {intr_local_disable(); KERN_DEBUG(__VA_ARGS__); intr_local_enable();}
#define say(...) {}

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
  lock->id = lock_ctr++;
  lock->free = LOCK_FREE;
  spinlock_init(&(lock->spinlock));
  queue_init(&(lock->waiting));
}

void lock_acquire(Lock *lock){
  spinlock_acquire(&(lock->spinlock));
  return;
  unsigned int pid, cpu;

  // required for storing within lock
  pid = get_curid();
  cpu = get_pcpu_idx();

  intr_local_disable();
  spinlock_acquire(&(lock->spinlock));

  if(lock->free == LOCK_BUSY){
    say("Process %u waiting on lock %d...\n", pid, lock->id);
    // wait on this lock
    enqueue(&(lock->waiting), pid);
    // sleep indefinitely (wait for release to be called)
    while(lock->holder != pid){
      spinlock_release(&(lock->spinlock));
      say("Process %u busywaiting on lock %d...\n", pid, lock->id);
      intr_local_enable();
      thread_yield();
      intr_local_disable();
      spinlock_acquire(&(lock->spinlock));
    }
    say("Process %u acquired lock %d...\n", pid, lock->id);
    // acquire lock spinlock again since thread_sleep releases it
    spinlock_acquire(&(lock->spinlock));
    // we've acquired the lock! It should not have been set to free in meantime
    if(lock->free != LOCK_BUSY)
      KERN_PANIC("Somehow lock became free while waiting on it.\n");
  }else{
    lock->free = LOCK_BUSY;
    lock->holder = pid;
  }
  
  spinlock_release(&(lock->spinlock));
  intr_local_enable();
}

void lock_release(Lock *lock){
  spinlock_release(&(lock->spinlock));
  return;
  intr_local_disable();
  spinlock_acquire(&(lock->spinlock));

  if(!queue_empty(&(lock->waiting))){
    // wake up the next thread waiting on this lock
    lock->holder = dequeue(&(lock->waiting));
    say("Process %u passed lock %d to %u...\n", get_curid(), lock->id, lock->holder);
  }else{
    // lock is ready to be picked up by anyone!
    lock->free = LOCK_FREE;
    say("Process %u released lock %d...\n", get_curid(), lock->id);
  }
  
  spinlock_release(&(lock->spinlock));
  intr_local_enable();
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
  while(cond->ticket != pid){
    spinlock_release(&(cond->spinlock));
    intr_local_enable();
    thread_yield();
    intr_local_disable();
    spinlock_acquire(&(cond->spinlock));
  }
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
  say("Process %u trying to put %d in buffer.\n", pid, data);
	lock_acquire(&(buffer->lock));
  say("Process %u acquired buffer's lock...\n", pid);

	//use while loop for wait
	//while full
	while ((buffer->tail - buffer->head) == BUFFER_SIZE) {
    say("Process %u waiting on full buffer\n", pid);
		cv_wait(&(buffer->full), &(buffer->lock));
	}
  say("Process %u is done waiting for buffer...\n", pid);

	//update vals
	buffer->buffer[buffer->tail % BUFFER_SIZE] = data;
	buffer->tail++;
  KERN_DEBUG("Process %u produced %d.\n", pid, data);
  say("Process %u added %d to the buffer and is signalling!...\n", pid, data);
	//signal anyone waiting on empty
	cv_signal(&(buffer->empty));
  say("Process %u signaled anyone waiting on empty buffer...\n", pid, data);
	lock_release(&(buffer->lock));
  say("Process %u released all its locks!\n", pid, data);
}

int buffer_get(BB *buffer) {
	unsigned int pid = get_curid();
  say("Process %u trying to get data from buffer.\n", pid);
	lock_acquire(&(buffer->lock));
  say("Process %u acquired buffer's lock...\n", pid);

	//wait for not empty
	while (buffer->head == buffer->tail) {
    say("Process %u waiting empty buffer...\n", pid);
		cv_wait(&(buffer->empty), &(buffer->lock));
	}
  say("Process %u is done waiting for buffer...\n", pid);

	//update vals
	int retval = buffer->buffer[buffer->head % BUFFER_SIZE];
  KERN_DEBUG("Process %u consumed %d.\n", pid, retval);
	buffer->head++;

	//signal to anyone waiting on full
	cv_signal(&(buffer->full));
	lock_release(&(buffer->lock));
  return retval;
}

