#include <lib/x86.h>

#include "import.h"

/**
 * Returns the page table entry corresponding to the virtual address,
 * according to the page structure of process # [proc_index].
 * Returns 0 if the mapping does not exist.
 */
unsigned int get_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int page_dir_entry_index = vaddr >> 22;
    unsigned int page_table_entry_index = ((vaddr >> 12) & 0x003FF);
    return get_ptbl_entry(proc_index, page_dir_entry_index, page_table_entry_index);

}         

// returns the page directory entry corresponding to the given virtual address
unsigned int get_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
     unsigned int page_dir_entry_index = vaddr >> 22;
     return get_pdir_entry(proc_index, page_dir_entry_index);
}

// removes the page table entry for the given virtual address
void rmv_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int page_dir_entry_index = vaddr >> 22;
    unsigned int page_table_entry_index = ((vaddr >> 12) & 0x003FF);
    rmv_ptbl_entry(proc_index, page_dir_entry_index, page_table_entry_index);
}

// removes the page directory entry for the given virtual address
void rmv_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    unsigned int page_dir_entry_index = vaddr >> 22;
    rmv_pdir_entry(proc_index, page_dir_entry_index);
}

// maps the virtual address [vaddr] to the physical page # [page_index] with permission [perm]
// you do not need to worry about the page directory entry. just map the page table entry.
void set_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr, unsigned int page_index, unsigned int perm)
{
    unsigned int page_dir_entry_index = vaddr >> 22;
    unsigned int page_table_entry_index = ((vaddr >> 12) & 0x003FF);
    set_ptbl_entry(proc_index, page_dir_entry_index, page_table_entry_index, page_index, perm);
}

// registers the mapping from [vaddr] to physical page # [page_index] in the page directory
void set_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr, unsigned int page_index)
{
     unsigned int page_dir_entry_index = vaddr >> 22;
     set_pdir_entry(proc_index, page_dir_entry_index, page_index);
}   

// initializes the identity page table
// the permission for the kernel memory should be PTE_P, PTE_W, and PTE_G,
// while the permission for the rest should be PTE_P and PTE_W.
void idptbl_init(unsigned int mbi_adr)
{
	container_init(mbi_adr);

	int i, j;
	for(i = 0; i < 1024; i++){
		for(j = 0; j < 1024; j++){
			set_ptbl_entry_identity(i, j, PTE_P | PTE_W | PTE_G);
		}
	}
}
