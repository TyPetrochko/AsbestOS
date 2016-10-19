#ifndef _KERN_PROC_PPROC_H_
#define _KERN_PROC_PPROC_H_

#ifdef _KERN_

struct kctx {
	void	*esp;
	unsigned int edi;
	unsigned int esi;
	unsigned int ebx;
	unsigned int ebp;
	void	*eip;
};

unsigned int container_get_quota(unsigned int id);
unsigned int container_get_usage(unsigned int id);

void copyPages(unsigned int parentProcess, unsigned int childProcess);
void hardCopy(unsigned int pid, unsigned int vaddr);

unsigned int get_curid(void);
void set_pdir_base(unsigned int);
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota);

#endif /* _KERN_ */

#endif /* !_KERN_PROC_PPROC_H_ */
