#ifdef _KERN_

#include <lib/spinlock.h>

// DATA STRUCTURES
typedef struct queue {
  unsigned int processes[NUM_IDS];
  unsigned int head;
  unsigned int tail;
} queue;

typedef struct Lock {
  int free;
  spinlock_t spinlock;
  queue waiting;
} Lock;

typedef struct CV {
  queue waiting;
} CV;

//functions from PThread

void thread_init(unsigned int mbi_addr);
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota);
void thread_yield(void);
void thread_update(void);
void sched_update(void);
void thread_sleep(spinlock_t* sl);
void thread_sleep_with_lock(Lock *lock);
void thread_wake(unsigned int pid);

//end functions from PThread


// UTIL -- QUEUE
void queue_init(queue *q);

void enqueue(queue *q, unsigned int pid);

unsigned int dequeue(queue *q);
int queue_empty(queue *q);

// UTIL -- LOCKS
void lock_init(Lock *lock);

void lock_aquire(Lock *lock);

void lock_release(Lock *lock);

//UTIL -- CV

void cv_init(CV *cond);

void cv_wait(CV *cond, unsigned int pid, Lock *lock);

void cv_signal(CV *cond);

#endif