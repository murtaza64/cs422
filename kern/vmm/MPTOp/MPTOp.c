#include <lib/x86.h>

#include "import.h"

#define VM_USERLO    0x40000000

/**
 * Returns the page table entry corresponding to the virtual address,
 * according to the page structure of process # [proc_index].
 * Returns 0 if the mapping does not exist.
 */
unsigned int get_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    unsigned int pde_index = (unsigned int) ((vaddr & 0xFFA00000) >> 22);
    unsigned int pdir_entry = get_pdir_entry(proc_index, pde_index);
    unsigned int pdir_perm = pdir_entry & 0x00000001;
    unsigned int pte_index = (unsigned int) ((vaddr & 0x003FF000) >> 12);
    unsigned int entry = get_ptbl_entry(proc_index, pde_index, pte_index);
    unsigned int present_perm = (unsigned int) (entry & 0x00000001);

    if (pdir_perm == 0 || present_perm == 0) {
        return 0;
    } else {
        return entry;
    }
}

// Returns the page directory entry corresponding to the given virtual address.
unsigned int get_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    unsigned int pde_index = (unsigned int) ((vaddr & 0xFFA00000) >> 22);
    return get_pdir_entry(proc_index, pde_index);
}

// Removes the page table entry for the given virtual address.
void rmv_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    unsigned int pde_index = (unsigned int) ((vaddr & 0xFFA00000) >> 22);
    unsigned int pte_index = (unsigned int) ((vaddr & 0x003FF000) >> 12);
    rmv_ptbl_entry(proc_index, pde_index, pte_index);
}

// Removes the page directory entry for the given virtual address.
void rmv_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr)
{
    // TODO
    unsigned int pde_index = (unsigned int) ((vaddr & 0xFFA00000) >> 22);
    rmv_pdir_entry(proc_index, pde_index);
}

// Maps the virtual address [vaddr] to the physical page # [page_index] with permission [perm].
// You do not need to worry about the page directory entry. just map the page table entry.
void set_ptbl_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index, unsigned int perm)
{
    // TODO
    unsigned int pde_index = (unsigned int) ((vaddr & 0xFFA00000) >> 22);
    unsigned int pte_index = (unsigned int) ((vaddr & 0x003FF000) >> 12);
    set_ptbl_entry(proc_index, pde_index, pte_index, page_index, perm);
}

// Registers the mapping from [vaddr] to physical page # [page_index] in the page directory.
void set_pdir_entry_by_va(unsigned int proc_index, unsigned int vaddr,
                          unsigned int page_index)
{
    // TODO
    unsigned int pde_index = (unsigned int) ((vaddr & 0xFFA00000) >> 22);
    set_pdir_entry(proc_index, pde_index, page_index);
}

// Initializes the identity page table.
// The permission for the kernel memory should be PTE_P, PTE_W, and PTE_G,
// While the permission for the rest should be PTE_P and PTE_W.
void idptbl_init(unsigned int mbi_addr)
{
    // TODO: Define your local variables here.
    unsigned int kern_perm, other_perm, pde_index, pte_index;

    container_init(mbi_addr);

    // TODO
    kern_perm = (PTE_P | PTE_W | PTE_G);
    other_perm = (PTE_P | PTE_W);

    for (pde_index = 0; pde_index < 1024; pde_index ++) {
        for (pte_index = 0; pte_index < 1024; pte_index ++) {
            if (pde_index < 0x100) { // VM_USERLO / 0x1000 / 0x400
                set_ptbl_entry_identity(pde_index, pte_index, kern_perm);
            }  else {
                set_ptbl_entry_identity(pde_index, pte_index, other_perm);
            }
        }
    }
}
