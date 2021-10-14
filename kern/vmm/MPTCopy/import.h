#ifndef _KERN_VMM_MPTCOPY_H_
#define _KERN_VMM_MPTCOPY_H_

#ifdef _KERN_

unsigned int get_pdir_entry(unsigned int proc_index, unsigned int pde_index);
void set_pdir_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int page_index);
unsigned int get_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                            unsigned int pte_index);
void set_ptbl_entry(unsigned int proc_index, unsigned int pde_index,
                    unsigned int pte_index, unsigned int page_index,
                    unsigned int perm);
                    
unsigned int alloc_ptbl(unsigned int proc_index, unsigned int vaddr);
unsigned int container_alloc(unsigned int id);

#endif  /* _KERN_ */

#endif  /* !_KERN_VMM_MPTCOPY_H_ */
