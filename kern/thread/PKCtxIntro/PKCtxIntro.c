#include <lib/x86.h>
#include <lib/debug.h>

/**
 * Kernel thread context.
 * When you switch to another kernel thread, you need to save
 * the current thread's states and restore the new thread's states.
 */
struct kctx {
	void	*esp;
	unsigned int edi;
	unsigned int esi;
	unsigned int ebx;
	unsigned int ebp;
	void	*eip;
};

//places to save the [NUM_IDS] kernel thread states.
struct kctx kctx_pool[NUM_IDS];

void kctx_set_esp(unsigned int pid, void *esp)
{
	kctx_pool[pid].esp = esp;
}

void kctx_set_eip(unsigned int pid, void *eip)
{
	kctx_pool[pid].eip = eip;
}

extern void cswitch(struct kctx *from_kctx, struct kctx *to_kctx);

void debug(unsigned int pid){
  KERN_DEBUG("debugging pid %u\n", pid);
  KERN_DEBUG("\tesp: %p\n", kctx_pool[pid].esp);
  KERN_DEBUG("\tedi: %u\n", kctx_pool[pid].edi);
  KERN_DEBUG("\tesi: %u\n", kctx_pool[pid].esi);
  KERN_DEBUG("\tebx: %u\n", kctx_pool[pid].ebx);
  KERN_DEBUG("\tebp: %u\n", kctx_pool[pid].ebp);
}

/**
 * Saves the states for thread # [from_pid] and restores the states
 * for thread # [to_pid].
 */
void kctx_switch(unsigned int from_pid, unsigned int to_pid)
{
  KERN_DEBUG("Should jump to: %p\n", (void*) kctx_pool[to_pid].eip);
  debug(from_pid);
  debug(to_pid);
	cswitch(&kctx_pool[from_pid], &kctx_pool[to_pid]);
  KERN_DEBUG("Should get here!\n");
}
