#ifndef _KERN_TRAP_TTRAPHANDLER_H_
#define _KERN_TRAP_TTRAPHANDLER_H_

#ifdef _KERN_

extern unsigned int last_pid[NUM_CPUS];

unsigned int syscall_get_arg1(void);
void set_pdir_base(unsigned int);
void proc_start_user(void);
void sched_update(void);

#endif /* _KERN_ */

#endif /* !_KERN_TRAP_TTRAPHANDLER_H_ */
