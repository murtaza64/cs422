#include <lib/debug.h>
#include "import.h"

#define PAGESIZE     4096
#define VM_USERLO    0x40000000
#define VM_USERHI    0xF0000000
#define VM_USERLO_PI (VM_USERLO / PAGESIZE)
#define VM_USERHI_PI (VM_USERHI / PAGESIZE)

/**
 * The initialization function for the allocation table AT.
 * It contains two major parts:
 * 1. Calculate the actual physical memory of the machine, and sets the number
 *    of physical pages (NUM_PAGES).
 * 2. Initializes the physical allocation table (AT) implemented in the MATIntro layer
 *    based on the information available in the physical memory map table.
 *    Review import.h in the current directory for the list of available
 *    getter and setter functions.
 */
void pmem_init(unsigned int mbi_addr)
{
    unsigned int nps;

    // TODO: Define your local variables here.
    unsigned int n_rows;

    unsigned int max_addr = 0x0;
    unsigned int start;
    unsigned int end;
    unsigned int row;
    unsigned int pg;
    unsigned int found;

    

    // Calls the lower layer initialization primitive.
    // The parameter mbi_addr should not be used in the further code.
    devinit(mbi_addr);

    /**
     * Calculate the total number of physical pages provided by the hardware and
     * store it into the local variable nps.
     * Hint: Think of it as the highest address in the ranges of the memory map table,
     *       divided by the page size.
     */
    //TODO
    
    n_rows = get_size();

    for (row = 0; row < n_rows; row++) {
        end = get_mms(row) + get_mml(row) - 1; //minus 1? good idea for test case?
        if (end > max_addr) {
            max_addr = end;
        }
    }

    nps = max_addr / PAGESIZE;


    set_nps(nps);  // Setting the value computed above to NUM_PAGES.

    /**
     * Initialization of the physical allocation table (AT).
     *
     * In CertiKOS, all addresses < VM_USERLO or >= VM_USERHI are reserved by the kernel.
     * That corresponds to the physical pages from 0 to VM_USERLO_PI - 1,
     * and from VM_USERHI_PI to NUM_PAGES - 1.
     * The rest of the pages that correspond to addresses [VM_USERLO, VM_USERHI)
     * can be used freely ONLY IF the entire page falls into one of the ranges in
     * the memory map table with the permission marked as usable.
     *
     * Hint:
     * 1. You have to initialize AT for all the page indices from 0 to NPS - 1.
     * 2. For the pages that are reserved by the kernel, simply set its permission to 1.
     *    Recall that the setter at_set_perm also marks the page as unallocated.
     *    Thus, you don't have to call another function to set the allocation flag.
     * 3. For the rest of the pages, explore the memory map table to set its permission
     *    accordingly. The permission should be set to 2 only if there is a range
     *    containing the entire page that is marked as available in the memory map table.
     *    Otherwise, it should be set to 0. Note that the ranges in the memory map are
     *    not aligned by pages, so it may be possible that for some pages, only some of
     *    the addresses are in a usable range. Currently, we do not utilize partial pages,
     *    so in that case, you should consider those pages as unavailable.
     */
    // TODO
    
    for (pg = 0; pg < nps; pg++) {
        if (pg < VM_USERLO_PI || pg >= VM_USERHI_PI) {
            at_set_perm(pg, 1);
        }
        else {
            found = 0;
            for (row = 0; row < n_rows; row++) {
                start = get_mms(row);
                end = start + get_mml(row) - 1;
                if (is_usable(row) && (start <= (pg * PAGESIZE)) && (end >= ((pg + 1) * PAGESIZE))) {
                    at_set_perm(pg, 2);
                    found = 1;
                    break;
                }
            }
            if (!found) {
                at_set_perm(pg, 0);
            }
        }
    }
}
