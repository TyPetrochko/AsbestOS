#include <lib/x86.h>

#include "import.h"

#define NEW_PERM (PTE_P | PTE_U | PTE_COW)
#define PT_PERM_PTU (PTE_P | PTE_W | PTE_U)
#define NUM_DIR 1024
#define NUM_TBL 1024


void copyPages(unsigned int parentProcess, unsigned int childProcess) {
	unsigned int parentPDE;
	unsigned int pageTableEntry;
	unsigned int pageIndex;
	unsigned int newIndex;
	unsigned int i;
	unsigned int j;
	//copy over pde
	for (i = 0; i < NUM_DIR; i++) {
		newIndex = container_alloc(childProcess);
		set_pdir_entry(childProcess, i, newIndex);
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