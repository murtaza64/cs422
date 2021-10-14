#include <lib/gcc.h>
#include <lib/x86.h>
#include <lib/debug.h>

#include "import.h"

#define PT_PERM_UP  0
#define PT_PERM_PTU (PTE_P | PTE_W | PTE_U)

#define PAGESIZE      4096
#define PDIRSIZE      (PAGESIZE * 1024)
#define VM_USERLO     0x40000000
#define VM_USERHI     0xF0000000
#define VM_USERLO_PDE (VM_USERLO / PDIRSIZE)
#define VM_USERHI_PDE (VM_USERHI / PDIRSIZE)


#define ADDR_MASK(x) ((unsigned int) x & 0xfffff000)

//Implement functions to copy the page directory and page tables of one process to another.
//Be careful to adjust the permissions correctly and copy only what is necessary (user space mappings).

// Copy the contents of page directory for pid `pid_from` to page structure for `pid_to`.
// Does not copy page tables for unmapped PDEs.
// Sets COW bit.
unsigned int copy_page_directory_structure(unsigned int pid_from, unsigned int pid_to){
    
    // Copy over page directory entries
    unsigned int pde_index, vaddr, new_ptbl, pte_index, addr, page_index, perm, entry;

    for (pde_index = VM_USERLO_PDE; pde_index < VM_USERHI_PDE; pde_index++) {
        entry = get_pdir_entry(pid_from, pde_index);
        if (entry != 0) {
            vaddr = pde_index << 22;
            if (alloc_ptbl(pid_to, vaddr) == 0) {
                return 0;
            }
            for (pte_index = 0; pte_index < 1024; pde_index++) {
                addr = get_ptbl_entry(pid_from, pde_index, pte_index);
                page_index = addr / PAGESIZE;
                perm = PTE_P | PTE_U | PTE_COW;
                set_ptbl_entry(pid_to, pde_index, pte_index, page_index, perm);
                set_ptbl_entry(pid_from, pde_index, pte_index, page_index, perm);
            }
        }

    }
    return 1;
}

unsigned int copy_mem_on_write(unsigned int pid, unsigned int vaddr) {
    unsigned int old_paddr_base, pte_index, pde_index, new_page_index;
    pte_index = vaddr >> 12;
    pde_index = vaddr >> 22; 
    old_paddr_base = ADDR_MASK(get_ptbl_entry(pid, pde_index, pte_index));
    //allocate new page
    new_page_index = container_alloc(pid);
    if (new_page_index == 0) {
        return 0;
    }
    //copy entire page
    unsigned int new_paddr_base = new_page_index * PAGESIZE;

    for (unsigned int idx = 0; idx < PAGESIZE; idx++) {
        ((char*) new_paddr_base)[idx] = ((char*) old_paddr_base)[idx];
    }
    set_ptbl_entry(pid, pde_index, pte_index, new_page_index, PT_PERM_PTU);
    return new_page_index;
}

