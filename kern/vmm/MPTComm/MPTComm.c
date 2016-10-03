#include <lib/x86.h>

#include "import.h"

/**
 * For each process from id 0 to NUM_IDS -1,
 * set the page directory entries sothat the kernel portion of the map as identity map,
 * and the rest of page directories are unmmaped.
 */
void pdir_init(unsigned int mbi_adr)
{
    // TODO: define your local variables here.

    idptbl_init(mbi_adr);

    // TODO
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
	unsigned int allocated_page = container_alloc(proc_index);
	if(allocated_page == 0)
		return 0;

	unsigned int pdir = get_pdir_entry_by_va(proc_index, vadr);
	set_pdir_entry(proc_index, vadr, allocated_page);
	for(int i = 0; i < 1024; i++)
		set_ptbl_entry_by_va(proc_index, vadr, 0, 0);

  return allocated_page;
}

// Reverse operation of alloc_ptbl.
// Removes corresponding page directory entry,
// and frees the page for the page table entries (with container_free).
void free_ptbl(unsigned int proc_index, unsigned int vadr)
{
	unsigned int page_to_free = get_pdir_entry_by_va(proc_index, vadr) >> 12;
	container_free(proc_index, page_to_free);
	rmv_ptbl_entry_by_va(proc_index, vadr);
}
