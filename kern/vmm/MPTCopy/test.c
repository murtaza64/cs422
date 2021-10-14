#include <lib/x86.h>
#include <lib/debug.h>
#include "export.h"

extern char *PDirPool[NUM_IDS][1024];
extern unsigned int IDPTbl[1024][1024];

int MPTCopy_test1()
{
    return 0;
}

int MPTCopy_test2()
{
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
int MPTCopy_test_own()
{
    // TODO (optional)
    // dprintf("own test passed.\n");
    return 0;
}

int test_MPTCopy()
{
    return MPTCopy_test1() + MPTCopy_test2() + MPTCopy_test_own();
}
