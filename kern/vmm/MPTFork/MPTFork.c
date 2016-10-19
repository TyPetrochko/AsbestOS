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
			pageIndex = pageTableEntry / PAGE_SIZE;
			set_ptbl_entry(parentProcess, i, j, pageIndex, NEW_PERM);
			set_ptbl_entry(childProcess, i, j, pageIndex, NEW_PERM);
		}
	}

}

void hardCopy(unsigned int parent, unsigned int child, unsigned int vaddr) {
	unsigned int parentPTE;
	unsigned int childPTE;
	unsigned int newPage;
	unsigned int pageIndex;
	unsigned int i;

	char * readPointer;
	char * writePointer;

    //get the ptblentry for the parent
    parentPTE = get_ptbl_entry_by_va(parent, vaddr);
    pageIndex = parentPTE / PAGE_SIZE;
    set_ptbl_entry_by_va(parent, vaddr, pageIndex, PT_PERM_PTU);

    //assign new page for pte for child;
    newPage = container_alloc(child);
    set_ptbl_entry_by_va(child, vaddr, newPage, PT_PERM_PTU);

    //copy over the actual info
    readPointer = (char *) pageIndex * PAGE_SIZE;
    writePointer = (char *) newPage * PAGE_SIZE;
    for (i = 0; i < 4096; i++) {
    	writePointer[i] = readPointer[i];
    }

}