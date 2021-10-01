#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PDE_KERN_LO  0x100 // VM_USERLO / 0x1000 / 0x400
#define PDE_KERN_HI  0x3c0

/**
 * Sets the entire page map for process 0 as the identity map.
 * Note that part of the task is already completed by pdir_init.
 */
void pdir_init_kern(unsigned int mbi_addr)
{
    // TODO: Define your local variables here.
    unsigned int i;

    pdir_init(mbi_addr);
    for (i = PDE_KERN_LO; i < PDE_KERN_HI; i++) {
        set_pdir_entry_identity(0, i);
    }

    //TODO
}

/**
 * Maps the physical page # [page_index] for the given virtual address with the given permission.
 * In the case when the page table for the page directory entry is not set up,
 * you need to allocate the page table first.
 * In the case of error, it returns the constant MagicNumber defined in lib/x86.h,
 * otherwise, it returns the physical page index registered in the page directory,
 * (the return value of get_pdir_entry_by_va or alloc_ptbl).
 */
unsigned int map_page(unsigned int proc_index, unsigned int vaddr,
                      unsigned int page_index, unsigned int perm)
{
    // TODO
    unsigned int ptbl_p_index;
    unsigned int entry = get_pdir_entry_by_va(proc_index, vaddr);
    if (!(entry & 0x1)) {
        ptbl_p_index = alloc_ptbl(proc_index, vaddr);
        if (ptbl_p_index == 0) {
            return MagicNumber;
        }
    }
    else {
        ptbl_p_index = entry >> 12;
    }
    set_ptbl_entry_by_va(proc_index, vaddr, page_index, perm);
    return ptbl_p_index;
}

/**
 * Remove the mapping for the given virtual address (with rmv_ptbl_entry_by_va).
 * You need to first make sure that the mapping is still valid,
 * e.g., by reading the page table entry for the virtual address.
 * Nothing should be done if the mapping no longer exists.
 * You do not need to unmap the page table from the page directory.
 * It should return the corresponding page table entry.
 */
unsigned int unmap_page(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    unsigned int entry = get_ptbl_entry_by_va(proc_index, vaddr);
    if (entry != 0) {
        set_ptbl_entry_by_va(proc_index, vaddr, 0, 0);
    }
    return entry;
}
