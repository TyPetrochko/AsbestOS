#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define NEW_PERM (PTE_P | PTE_U | PTE_COW)
#define PT_PERM_PTU (PTE_P | PTE_W | PTE_U)
#define NUM_DIR 1024
#define NUM_TBL 1024

#define USER_DIR_LO 256
#define USER_DIR_HI 960

void copyPages(unsigned int parentProcess, unsigned int childProcess) {
  unsigned int parentPDE;
	unsigned int pageTableEntry;
	unsigned int pageIndex;
	unsigned int newIndex;
	unsigned int i;
	unsigned int j;

  unsigned int new_ptbl;
  //copy over pde
	for (i = USER_DIR_LO; i < USER_DIR_HI; i++) {
		// newIndex = container_alloc(childProcess);
    new_ptbl = alloc_ptbl(childProcess, i * 4096 * 1024);
    
    if(new_ptbl == 0)
      KERN_PANIC("Could not allocate page table!\n");

		// set_pdir_entry(childProcess, i, newIndex);
		//set permissions of ptbls
		for (j = 0; j < NUM_TBL; j++) {
			pageTableEntry = get_ptbl_entry(parentProcess, i, j);
			pageIndex = pageTableEntry / PAGESIZE;
			set_ptbl_entry(parentProcess, i, j, pageIndex, NEW_PERM);
			set_ptbl_entry(childProcess, i, j, pageIndex, NEW_PERM);
		}
	}
}

void hardCopy(unsigned int pid, unsigned int vaddr) {
  KERN_DEBUG("Hardcopying va = 0x%08x, pid = %d\n", vaddr, pid);
	unsigned int entry;
	unsigned int newPageIndex;
	unsigned int oldPageIndex;
	unsigned int i;

	char * readPointer;
	char * writePointer;
  
  //get the old ptblentry 
  entry = get_ptbl_entry_by_va(pid, vaddr);
  oldPageIndex = entry / PAGESIZE;

  //assign new page for pte;
  newPageIndex = container_alloc(pid);
  set_ptbl_entry_by_va(pid, vaddr, newPageIndex, PT_PERM_PTU);

  //copy over the actual info
  readPointer = (char *) (oldPageIndex * PAGESIZE);
  writePointer = (char *) (newPageIndex * PAGESIZE);
  for (i = 0; i < 4096; i++) {
    writePointer[i] = readPointer[i];
  }
}
