#include "lib/x86.h"

#include "import.h"

/**
 * Initializes all the thread queues with
 * tqueue_init_at_id.
 */
void tqueue_init(unsigned int mbi_addr)
{
  unsigned int i;

  tcb_init(mbi_addr);

  for (i = 0; i < NUM_IDS + 1; i++) {
  	tqueue_init_at_id(i);
  }
}

/**
 * Insert the TCB #pid into the tail of the thread queue #chid.
 * Recall that the doubly linked list is index based.
 * So you only need to insert the index.
 * Hint: there are multiple cases in this function.
 */
void tqueue_enqueue(unsigned int chid, unsigned int pid)
{
  unsigned int tailPid;

  tailPid = tqueue_get_tail(chid);
  //if there is nothing in the queue
  if (tailPid == NUM_IDS) {
  	tqueue_set_head(chid, pid);
  } else {
  	//update the doubly linked list
  	tcb_set_prev(pid, tailPid);
  	tcb_set_next(tailPid, pid); 	
  }

  //regardless, update the tail
  tqueue_set_tail(chid, pid);
}

/**
 * Reverse action of tqueue_enqueue, i.g., pops a TCB from the head of specified queue.
 * It returns the poped thread's id, or NUM_IDS if the queue is empty.
 * Hint: there are mutiple cases in this function.
 */
unsigned int tqueue_dequeue(unsigned int chid)
{

  unsigned int headPid;
  unsigned int nextPid;

  headPid = tqueue_get_head(chid);
  //if the queue is not empty
  if (headPid != NUM_IDS) {
  	//get the 2nd pid in queue
  	nextPid = tcb_get_next(headPid);
  	if (nextPid != NUM_IDS) {
  		//detach 
  		tcb_set_prev(nextPid, NUM_IDS);
  	} else {
  		//now queue is empty
  		tqueue_set_tail(chid, NUM_IDS);
  	}
  	//detach head
  	tcb_set_next(headPid, NUM_IDS);
  	//set the new head
  	tqueue_set_head(chid, nextPid);
  }
  //regardless, return the head
  return headPid;
}

/**
 * Removes the TCB #pid from the queue #chid.
 * Hint: there are many cases in this function.
 */
void tqueue_remove(unsigned int chid, unsigned int pid)
{
  unsigned int head;
  unsigned int tail;
  unsigned int prev;
  unsigned int next;

  head = tqueue_get_head(chid);
  tail = tqueue_get_tail(chid);
  next = tcb_get_next(pid);
  prev = tcb_get_prev(pid);

  //remove self from list
  tcb_set_next(pid, NUM_IDS);
  tcb_set_prev(pid, NUM_IDS);
  if (next != NUM_IDS) {
    tcb_set_prev(next, prev);
  }
  if (prev != NUM_IDS) {
    tcb_set_next(prev, next);
  }
  //fix head and tail if necessary
  if (pid == head) {
    tqueue_set_head(chid, next);
  }
  if (pid == tail) {
    tqueue_set_tail(chid, prev);
  }
  //TODO: this assumes pid is in chid? should we check?
}
