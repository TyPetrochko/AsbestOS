#ifdef _KERN_

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


// UTIL -- QUEUE
void queue_init(queue *q);

void enqueue(queue *q, unsigned int pid);

unsigned int dequeue(queue *q);
int queue_empty(queue *q);

// UTIL -- LOCKS
void lock_init(Lock *lock);

void lock_aquire(Lock *lock, tf_t *tf);

void lock_release(Lock *lock);

//UTIL -- CV

void cv_init(CV *cond);

void cv_wait(CV *cond, unsigned int pid, Lock, *lock);

void cv_signal(CV *cond);

#endif