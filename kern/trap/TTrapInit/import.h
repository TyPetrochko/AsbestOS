#ifndef _KERN_TRAP_TTRAPINIT_H_
#define _KERN_TRAP_TTRAPINIT_H_

#ifdef _KERN_

#include <lib/trap.h>
#include <lib/x86.h>

extern unsigned int last_pid[NUM_CPUS];

void syscall_dispatch(tf_t *);
void exception_handler(tf_t *);
void interrupt_handler(tf_t *);

#endif /* _KERN_ */

#endif /* !_KERN_TRAP_TTRAPINIT_H_ */
