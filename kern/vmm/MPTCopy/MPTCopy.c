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
void copy_page_directory_structure(unsigned int pid_from, unsigned int pid_to){
    
    // Copy over page directory entries
    unsigned int pde_index;
    for (pde_index = VM_USERLO_PDE; pde_index < VM_USERHI_PDE; pde_index++) {
        unsigned int entry = get_pdir_entry(pid_from, pde_index);
        set_pdir_entry(pid_to, pde_index, entry);
        if (entry != 0) {
            unsigned int pte_index;            
            for (pte_index = 0; pte_index < 1024; pde_index++) {
                unsigned int addr = get_ptbl_entry(pid_from, pde_index, pte_index);
                unsigned int page_index = addr / PAGESIZE;
                unsigned int perm = PTE_P | PTE_U | PTE_COW;
                set_ptbl_entry(pid_to, pde_index, pte_index, page_index, perm);
            }
        }

    }
    
}

