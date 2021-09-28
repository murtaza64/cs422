#include <lib/x86.h>

#include "import.h"
#define VM_USERLO = 0x40000000;

/**
 * For each process from id 0 to NUM_IDS - 1,
 * set up the page directory entries so that the kernel portion of the map is
 * the identity map, and the rest of the page directories are unmapped.
 */
void pdir_init(unsigned int mbi_addr)
{
    // TODO: Define your local variables here.
    unsigned int proc;
    unsigned int pde_index;
    idptbl_init(mbi_addr);
    for (proc = 0; proc < NUM_IDS; proc++) {
        for (pde_index = 0; pde_index < 1024; pde_index++) {
            if (pde_index < 0x100) { // VM_USERLO / 0x1000 / 0x400
                set_pdir_entry_identity(proc, pde_index);
            }
        }
    }
    // TODO
}

/**
 * Allocates a page (with container_alloc) for the page table,
 * and registers it in the page directory for the given virtual address,
 * and clears (set to 0) all page table entries for this newly mapped page table.
 * It returns the page index of the newly allocated physical page.
 * In the case when there's no physical page available, it returns 0.
 */
unsigned int alloc_ptbl(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    if (!container_can_consume(proc_index, 1)) {
        return 0;
    }
    int pg = container_alloc(proc_index);
    set_pdir_entry_by_va(proc_index, vaddr, pg);
    for (int pte = 0; pte < 1024; pte++) {
        ((unsigned int *) (pg * 4096))[pte] = (unsigned int) 0;
    }
    return pg;
}

// Reverse operation of alloc_ptbl.
// Removes corresponding the page directory entry,
// and frees the page for the page table entries (with container_free).
void free_ptbl(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    int pg_index = vaddr / 4096;
    rmv_pdir_entry_by_va(proc_index, vaddr);
    container_free(proc_index, pg_index);
}
