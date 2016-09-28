#include <lib/debug.h>
#include "import.h"

#define PAGESIZE	4096
#define VM_USERLO	0x40000000
#define VM_USERHI	0xF0000000
#define VM_USERLO_PI	(VM_USERLO / PAGESIZE)
#define VM_USERHI_PI	(VM_USERHI / PAGESIZE)

// round an address to the next highest page offset
// 		i.e. ceiling_page(PAGESIZE - 15) = PAGESIZE
unsigned int ceiling_page(unsigned int addr);
// round an address to the next lowest page offset
// 		i.e. floor_page(PAGESIZE + 15) = PAGESIZE
unsigned int floor_page(unsigned int addr);

/**
 * The initialization function for the allocation table AT.
 * It contains two major parts:
 * 1. Calculate the actual physical memory of the machine, and sets the number of physical pages (NUM_PAGES).
 * 2. Initializes the physical allocation table (AT) implemented in MATIntro layer, based on the
 *    information available in the physical memory map table.
 *    Review import.h in the current directory for the list of avaiable getter and setter functions.
 */
void
pmem_init(unsigned int mbi_addr)
{
  unsigned int nps;
	unsigned int num_rows;
	unsigned int highest_addr;

  //Calls the lower layer initializatin primitives.
  //The parameter mbi_addr shell not be used in the further code.
	devinit(mbi_addr);
  /**
   * Calculate the number of actual number of avaiable physical pages and store it into the local varaible nps.
   * Hint: Think of it as the highest address possible in the ranges of the memory map table,
   *       divided by the page size.
   */

	num_rows = get_size();

	// Iterate over all rows in physical memory map and find the highest address possible
	for(unsigned int i = 0; i < num_rows; i++){
		if(is_usable(i) && get_mms(i) + get_mml(i) > highest_addr){
			highest_addr = get_mms(i) + get_mml(i);
			nps = highest_addr / PAGESIZE;
		}
	}

	set_nps(nps); // Setting the value computed above to NUM_PAGES.
	
	/**
   * Initialization of the physical allocation table (AT).
   *
   * In CertiKOS, the entire addresses < VM_USERLO or >= VM_USERHI are reserved by the kernel.
   * That corresponds to the physical pages from 0 to VM_USERLO_PI-1, and from VM_USERHI_PI to NUM_PAGES-1.
   * The rest of pages that correspond to addresses [VM_USERLO, VM_USERHI), can be used freely ONLY IF
   * the entire page falls into one of the ranges in the memory map table with the permission marked as usable.
   *
   * Hint:
   * 1. You have to initialize AT for all the page indices from 0 to NPS - 1.
   * 2. For the pages that are reserved by the kernel, simply set its permission to 1.
   *    Recall that the setter at_set_perm also marks the page as unallocated. 
   *    Thus, you don't have to call another function to set the allocation flag.
   * 3. For the rest of the pages, explore the memory map table to set its permission accordingly.
   *    The permission should be set to 2 only if there is a range containing the entire page that
   *    is marked as available in the memory map table. O.w., it should be set to 0.
   *    Note that the ranges in the memory map are not aligned by pages.
   *    So it may be possible that for some pages, only part of the addresses are in the ranges.
   *    Currently, we do not utilize partial pages, so in that case, you should consider those pages as unavailble.
   * 4. Every page in the allocation table shold be initialized.
   *    But the ranges in the momory map table may not cover the entire available address space.
   *    That means there may be some gaps between the ranges.
   *    You should still set the permission of those pages in allocation table to 0.
   */

	// init unusable range between num_pages and 2^20
	unsigned int at_size = 1 << 20;
	for(int i = nps; i < at_size; i++){
		at_set_perm(i, 0);
	}

	// init all pages to BIOS-only by default (memory map may not cover them)
	for(int i = VM_USERLO_PI; i < VM_USERHI_PI; i++){
		at_set_perm(i, 0);
	}

	// map the rest of the pages by range, according to mem-map
	unsigned int start;
	unsigned int end;
	for(unsigned int i = 0; i < num_rows; i++){
		if(is_usable(i)){
			unsigned int bottom_addr = get_mms(i);
			unsigned int top_addr = bottom_addr + get_mml(i);
			unsigned int ceil = ceiling_page(bottom_addr);
			unsigned int floor = floor_page(top_addr);

			start = (ceiling_page(get_mms(i))) / PAGESIZE;
			end = (floor_page(get_mms(i) + get_mml(i))) / PAGESIZE;
			for(unsigned int j = start; j <= end; j++){
				at_set_perm(j, 2);
			}
		}
	}

	// init low-end kernel-reserved pages
	for(int i = 0; i < VM_USERLO_PI; i++){
		at_set_perm(i, 1);
	}

	// init high-end kernel-reserved pages
	for(int i = VM_USERHI_PI; i < nps; i++){
		at_set_perm(i, 1);
	}

}

unsigned int ceiling_page(unsigned int addr){
	if(addr % PAGESIZE == 0){
		return addr;
	}else{
		return floor_page(addr) + PAGESIZE;
	}
}

unsigned int floor_page(unsigned int addr){
	return addr - (addr % PAGESIZE);
}
