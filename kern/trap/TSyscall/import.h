#ifndef _KERN_TRAP_TSYSCALL_H_
#define _KERN_TRAP_TSYSCALL_H_

#ifdef _KERN_

extern tf_t uctx_pool[NUM_IDS];

unsigned int container_get_quota(unsigned int id);
unsigned int container_get_usage(unsigned int id);
unsigned int container_can_consume(unsigned int id, unsigned int n);
unsigned int container_get_nchildren(unsigned int id);

unsigned int get_curid(void);
unsigned int syscall_get_arg1(void);
unsigned int syscall_get_arg2(void);
unsigned int syscall_get_arg3(void);
unsigned int syscall_get_arg4(void);
unsigned int syscall_get_arg5(void);
unsigned int syscall_get_arg6(void);

void syscall_set_errno(unsigned int errno);
void syscall_set_retval1(unsigned int retval);
void syscall_set_retval2(unsigned int retval);
void syscall_set_retval3(unsigned int retval);
void syscall_set_retval4(unsigned int retval);
void syscall_set_retval5(unsigned int retval);

// Special funcs for fork()
void syscall_set_pid_errno(unsigned int pid, unsigned int errno);
void syscall_set_pid_retval(unsigned int pid, unsigned int retval);

unsigned int proc_create(void *elf_addr, unsigned int quota);
unsigned int proc_fork(void);
void thread_yield(void);


#endif /* _KERN_ */

#endif /* !_KERN_TRAP_TSYSCALL_H_ */
