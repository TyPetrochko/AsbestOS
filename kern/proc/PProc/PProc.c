#include <lib/elf.h>
#include <lib/debug.h>
#include <lib/gcc.h>
#include <lib/seg.h>
#include <lib/trap.h>
#include <lib/x86.h>

#include "import.h"

extern tf_t uctx_pool[NUM_IDS];
extern struct kctx kctx_pool[NUM_IDS];

extern char STACK_LOC[NUM_IDS][PAGESIZE];

void proc_start_user(void)
{
	unsigned int cur_pid = get_curid();
	tss_switch(cur_pid);
	set_pdir_base(cur_pid);

	trap_return((void *) &uctx_pool[cur_pid]);
}

unsigned int proc_create(void *elf_addr, unsigned int quota)
{
	unsigned int pid, id;

  id = get_curid();
	pid = thread_spawn((void *) proc_start_user, id, quota);

	elf_load(elf_addr, pid);

  uctx_pool[pid].es = CPU_GDT_UDATA | 3;
  uctx_pool[pid].ds = CPU_GDT_UDATA | 3;
  uctx_pool[pid].cs = CPU_GDT_UCODE | 3;
  uctx_pool[pid].ss = CPU_GDT_UDATA | 3;
  uctx_pool[pid].esp = VM_USERHI;
  uctx_pool[pid].eflags = FL_IF;
  uctx_pool[pid].eip = elf_entry(elf_addr);

	return pid;
}

unsigned int proc_fork()
{
	unsigned int parent_id, child_id, quota;

  // get quota of new process
  parent_id = get_curid();
  quota = container_get_quota(parent_id) / 2;

  // spawn the new process
	child_id  = thread_spawn(kctx_pool[parent_id], parent_id, quota);

  // set user context values
  uctx_pool[pid].es     = uctx_pool[pid].es;
  uctx_pool[pid].ds     = uctx_pool[pid].ds;
  uctx_pool[pid].cs     = uctx_pool[pid].cs;
  uctx_pool[pid].ss     = uctx_pool[pid].ss;
  uctx_pool[pid].esp    = uctx_pool[pid].esp;
  uctx_pool[pid].eflags = uctx_pool[pid].eflags;
  uctx_pool[pid].eip    = uctx_pool[pid].eip;

  // duplicate memory!
  copyPages(parent_id, child_id);

	return pid;
}

