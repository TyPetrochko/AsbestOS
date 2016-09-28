#include <lib/debug.h>
#include <pmm/MATIntro/export.h>
#include "export.h"

int MATOp_test1()
{
  int page_index = palloc();
  if (page_index < 262144) {
    pfree(page_index);
    dprintf("test 1 failed. min is 262144, got %u \n", page_index);
    return 1;
  }
  if (at_is_norm(page_index) != 1) {
    pfree(page_index);
    dprintf("test 1 failed.\n");
    return 1;
  }
  if (at_is_allocated(page_index) != 1) {
    pfree(page_index);
    dprintf("test 1 failed.\n");
    return 1;
  }
  pfree(page_index);
  if (at_is_allocated(page_index) != 0) {
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
int MATOp_test_own()
{
	unsigned int pages [1 << 20];
	unsigned int index = 0;

	// use up all memory
	unsigned int tmp;
	while((tmp = palloc()) != 0){
		pages[index++] = tmp;

		// abort if allocation failed
		if(!at_is_allocated(tmp)){
			dprintf("couldn't allocate page %u", pages[tmp]);
			while(index > 0) pfree(pages[--index]);
			return 1;
		}
	}

	// free the last allocated page
	pfree(pages[--index]);

	// make sure it was actually freed
	if(at_is_allocated(pages[index])){
		dprintf("couldn't free page %u\n", pages[index]);
		while(index > 0) pfree(pages[--index]);
		return 1;
	}

	// make sure that the last freed page was re-allocated
	if((tmp = palloc()) != pages[index]){
		dprintf("allocated page %u, should be %u\n", tmp, pages[index]);
		pfree(tmp);
		while(index > 0) pfree(pages[--index]);
		return 1;
	}

	// make sure it was actually allocated
	if(!at_is_allocated(pages[index])){
		dprintf("couldn't allocated page %u\n", pages[index]);
		while(index > 0) pfree(pages[--index]);
		return 1;
	}

	// free all used pages
	index++;
	while(index > 0){
		// make sure this page is allocated
		if(!at_is_allocated(pages[--index])){
			dprintf("page %u was already freed\n", pages[index]);
			while(index > 0) pfree(pages[--index]);
			return 1;
		}
		// free this page
		pfree(pages[index]);
		// make sure this page was freed
		if(at_is_allocated(pages[index])){
			dprintf("couldn't free page %u\n", pages[index]);
			while(index > 0) pfree(pages[--index]);
			return 1;
		}
	}

  dprintf("own test passed.\n");
  return 0;
}

int test_MATOp()
{
  return MATOp_test1() + MATOp_test_own();
}
