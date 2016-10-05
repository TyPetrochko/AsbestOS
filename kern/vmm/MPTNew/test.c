#include <lib/debug.h>
#include "export.h"

int MPTNew_test1()
{
  unsigned int vaddr = 4096*1024*400;
  container_split(0, 100);
  if (get_ptbl_entry_by_va(1, vaddr) != 0) {
    dprintf("test 1 failed.\n");
    return 1;
  }
  if (get_pdir_entry_by_va(1, vaddr) != 0) {
    dprintf("test 1 failed.\n");
    return 1;
  }
  alloc_page(1, vaddr, 7);
  if (get_ptbl_entry_by_va(1, vaddr) == 0) {
    dprintf("test 1 failed.\n");
    return 1;
  }
  if (get_pdir_entry_by_va(1, vaddr) == 0) {
    dprintf("test 1 failed.\n");
    return 1;
  }
  dprintf("test 1 passed.\n");
  return 0;
}

/**
 * Write Your Own Test Script (optional)
 *
 * Come up with your own interesting test cases to challenge your classmates!
 * In addition to the provided simple tests, selected (correct and interesting) test functions
 * will be used in the actual grading of the lab!
 * Your test function itself will not be graded. So don't be afraid of submitting a wrong script.
 *
 * The test function should return 0 for passing the test and a non-zero code for failing the test.
 * Be extra careful to make sure that if you overwrite some of the kernel data, they are set back to
 * the original value. O.w., it may make the future test scripts to fail even if you implement all
 * the functions correctly.
 */
int MPTNew_test_own()
{
  //confirm that alloc_page doesn't give the same physical page more than once
  unsigned int vaddr1 = 4096*1024*400;
  unsigned int vaddr2 = 4096*1024*600;
  unsigned int vaddr3 = 4096*1024*800;
  unsigned int page1 = alloc_page(1, vaddr1, 7);
  unsigned int page2 = alloc_page(2, vaddr2, 7);
  unsigned int page3 = alloc_page(2, vaddr3, 7);
  if (page1 == page2 || page2 == page3 || page3 == page1) {
    dprintf("test failed");
    return 1;
  }
  dprintf("own test passed.\n");
  return 0;
}

int test_MPTNew()
{
  return MPTNew_test1() + MPTNew_test_own();
}
