#ifndef _KERN_VMM_MPTCOPY_H_
#define _KERN_VMM_MPTCOPY_H_

#ifdef _KERN_

unsigned int copy_page_directory_structure(unsigned int pid_from, unsigned int pid_to);
unsigned int copy_mem_on_write(unsigned int pid, unsigned int vaddr);

#endif  /* _KERN_ */

#endif  /* !_KERN_VMM_MPTCOPY_H_ */
