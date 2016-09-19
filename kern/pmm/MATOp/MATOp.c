#include <lib/debug.h>
#include "import.h"
#define AT_MAX (1 << 20)
#define PAGESIZE	4096
#define VM_USERLO	0x40000000
#define VM_USERLO_PI	(VM_USERLO / PAGESIZE)

unsigned int next_page(unsigned int);

/**
 * Allocation of a physical page.
 *
 * 1. First, implement a naive page allocator that scans the allocation table (AT) 
 *    using the functions defined in import.h to find the first unallocated page
 *    with usable permission.
 *    (Q: Do you have to scan allocation table from index 0? Recall how you have
 *    initialized the table in pmem_init.)
 *    Then mark the page as allocated in the allocation table and return the page
 *    index of the page found. In the case when there is no avaiable page found,
 *    return 0.
 * 2. Optimize the code with the memorization techniques so that you do not have to
 *    scan the allocation table from scratch every time.
 */
unsigned int
palloc()
{
	// next available page to allocate
	static unsigned int curr_page = VM_USERLO_PI;

	// find the next available page
	unsigned int beginning = curr_page;
	while(at_is_norm(curr_page) == 0 || at_is_allocated(curr_page) > 0) {
		curr_page = next_page(curr_page);
		if(curr_page == beginning) return 0; /* no free memory */
	}

	// allocate page
	at_set_allocated(curr_page, 1);
	return curr_page;
} 


/**
 * Free of a physical page.
 *
 * This function marks the page with given index as unallocated
 * in the allocation table.
 *
 * Hint: Simple.
 */
void
pfree(unsigned int pfree_index)
{
	at_set_allocated(pfree_index, 0);
}

// return the next page to check for availability
unsigned int next_page(unsigned int i){
	i = (i + 1) % AT_MAX; /* don't go above highest address available */

	if(i < VM_USERLO_PI){ /* ignore pages below VM_USER_LO_PI */
		i = VM_USERLO_PI;
	}
	return i;
}

