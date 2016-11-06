#ifdef _KERN_

#include <lib/spinlock.h>

#define BUFFER_SIZE (15)

// DATA STRUCTURES
typedef struct queue {
  unsigned int processes[NUM_IDS];
  unsigned int head;
  unsigned int tail;
} queue;

typedef struct Lock {
  spinlock_t spinlock;
} Lock;

typedef struct CV {
  spinlock_t spinlock; // used to protect ticket
  unsigned int ticket; // used to signal waiting pids...
  queue waiting;
} CV;

typedef struct bounded_buffer {
	int head;
	int tail;
	int buffer[BUFFER_SIZE];
	Lock lock;
	CV empty;
	CV full;
} BB;

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

void lock_acquire(Lock *lock);

void lock_release(Lock *lock);

//UTIL -- CV

void cv_init(CV *cond);

void cv_wait(CV *cond, Lock *lock);

void cv_signal(CV *cond);

void buffer_init(BB *buffer);

void buffer_put(int data, BB *buffer);

int buffer_get(BB *buffer);

//other functions to import
unsigned int get_curid();

#endif
