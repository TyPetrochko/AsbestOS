#include <lib/x86.h>

#include "import.h"

#define PAGESIZE    4096
#define VM_USERLO   0x40000000
#define VM_USERHI   0xF0000000
#define VM_USERLO_PI    (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI    (VM_USERHI / PAGESIZE)

#define NUM_PG_DIRS 1024
#define NUM_PG_TBLS 1024

/**
 * For each process from id 0 to NUM_IDS -1,
 * set the page directory entries sothat the kernel portion of the map as identity map,
 * and the rest of page directories are unmmaped.
 */

void pdir_init(unsigned int mbi_adr)
{

    idptbl_init(mbi_adr);

    for (int i = 0; i < NUM_IDS; i++) {
    	for (int j = 0; j < VM_USERLO_PI / NUM_PG_DIRS; j++) {
    		set_pdir_entry(i, j);
    	}
    	for (int j = VM_USERHI_PI /NUM_PG_DIRS; j < VM_USERHI_PI / NUM_PG_DIRS; j++) {
    		set_pdir_entry_identity(i, j, 0);
    	}
    	for (int j = VM_USERHI_PI / NUM_PG_DIRS; j < NUM_PG_DIRS; j++) {
    		set_pdir_entry(i, j);
    	}
    }
}

/**
 * Allocates a page (with container_alloc) for the page table,
 * and registers it in page directory for the given virtual address,
 * and clears (set to 0) the whole page table entries for this newly mapped page table.
 * It returns the page index of the newly allocated physical page.
 * In the case when there's no physical page available, it returns 0.
 */
unsigned int alloc_ptbl(unsigned int proc_index, unsigned int vadr)
{
  // TODO
  return 0;
}

// Reverse operation of alloc_ptbl.
// Removes corresponding page directory entry,
// and frees the page for the page table entries (with container_free).
void free_ptbl(unsigned int proc_index, unsigned int vadr)
{
  unsigned int page_index = get_pdir
}
