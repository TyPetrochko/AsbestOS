#include <lib/debug.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/trap.h>
#include <lib/syscall.h>

#include "import.h"

static char sys_buf[NUM_IDS][PAGESIZE];

/**
 * Copies a string from user into buffer and prints it to the screen.
 * This is called by the user level "printf" library as a system call.
 */
void sys_puts(void)
{
	unsigned int cur_pid;
	unsigned int str_uva, str_len;
	unsigned int remain, cur_pos, nbytes;

	cur_pid = get_curid();
	str_uva = syscall_get_arg2();
	str_len = syscall_get_arg3();

	if (!(VM_USERLO <= str_uva && str_uva + str_len <= VM_USERHI)) {
		syscall_set_errno(E_INVAL_ADDR);
		return;
	}

	remain = str_len;
	cur_pos = str_uva;

	while (remain) {
		if (remain < PAGESIZE - 1)
			nbytes = remain;
		else
			nbytes = PAGESIZE - 1;

		if (pt_copyin(cur_pid,
			      cur_pos, sys_buf[cur_pid], nbytes) != nbytes) {
			syscall_set_errno(E_MEM);
			return;
		}

		sys_buf[cur_pid][nbytes] = '\0';
		KERN_INFO("%s", sys_buf[cur_pid]);

		remain -= nbytes;
		cur_pos += nbytes;
	}

	syscall_set_errno(E_SUCC);
}

extern uint8_t _binary___obj_user_pingpong_ping_start[];
extern uint8_t _binary___obj_user_pingpong_pong_start[];
extern uint8_t _binary___obj_user_pingpong_ding_start[];
extern uint8_t _binary___obj_user_fork_fork_start[];

/**
 * Spawns a new child process.
 * The user level library function sys_spawn (defined in user/include/syscall.h)
 * takes two arguments [elf_id] and [quota], and returns the new child process id
 * or NUM_IDS (as failure), with appropriate error number.
 * Currently, we have three user processes defined in user/pingpong/ directory,
 * ping, pong, and ding.
 * The linker ELF addresses for those compiled binaries are defined above.
 * Since we do not yet have a file system implemented in mCertiKOS,
 * we statically loading the ELF binraries in to the memory based on the
 * first parameter [elf_id], i.e., ping, pong, ding and fork corresponds to
 * the elf_id of 1, 2, 3, and 4, respectively.
 * If the parameter [elf_id] is none of those three, then it should return
 * NUM_IDS with the error number E_INVAL_PID. The same error case apply
 * when the proc_create fails.
 * Otherwise, you mark it as successful, and return the new child process id.
 */
void sys_spawn(void)
{
  // get elf_id, quota
  //
  // check that elf_id is 1, 2, or 3, else return NUM_IDS and set errno E_INVAL_PID
  //
  // check container_can_consume get_curid(), quota
  //    -> if not, do fail (see above)
  // also check if current proc can make another child, i.e. container_get_nchildren(get_curid()) < MAX_CHILDREN
  //
  // call proc_create() on proper elf binary, with quota
  //
  // check that return value from proc_create() does not exceed NUM_IDS
  //    -> if so, do fail
  // 
  // if hasn't failed thus far
  //    -> set error code to no error
  //    -> set return value 1 to the newly created proc_id
  //
  unsigned int elf_id, quota, proc_id;
  
  // 0-indexed map of elf binarioes
  void *elf_addrs[3] = 
  {
    _binary___obj_user_pingpong_ping_start,
    _binary___obj_user_pingpong_pong_start,
    _binary___obj_user_pingpong_ding_start
  };
  
  elf_id  = syscall_get_arg2();
  quota   = syscall_get_arg3();

  // validations (see piazza question on this)
  if(elf_id < 1 || elf_id > 3)
    goto sys_spawn_failed;

  else if(!container_can_consume(get_curid(), quota))
    goto sys_spawn_failed;

  else if(container_get_nchildren(get_curid()) >= MAX_CHILDREN)
    goto sys_spawn_failed;

  // Make the new process!
  proc_id = proc_create(elf_addrs[elf_id - 1], quota);

  // more validations
  if(proc_id > NUM_IDS)
    goto sys_spawn_failed;

  // it worked!
  syscall_set_errno(E_SUCC);
  syscall_set_retval1(proc_id);
  return;

  // we failed somewhere, notify caller!
sys_spawn_failed:
  syscall_set_errno(E_INVAL_PID);
  syscall_set_retval1(NUM_IDS);
}

/**
 * Yields to another thread/process.
 * The user level library function sys_yield (defined in user/include/syscall.h)
 * does not take any argument and does not have any return values.
 * Do not forget to set the error number as E_SUCC.
 */
void sys_yield(void)
{
  syscall_set_errno(E_SUCC);
  thread_yield();
}

// Your implementation of fork
void sys_fork()
{
  unsigned int child_pid = proc_fork();

  // TODO errorcheck!!!
 
  // set the new process's
  syscall_set_pid_errno(pid, E_SUCC);
  syscall_set_pid_ret_val(pid, 0);

  syscall_set_errno(E_SUCC);
  syscall_set_retval1(child_pid);
}
