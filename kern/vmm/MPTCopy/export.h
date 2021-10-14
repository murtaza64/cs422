#ifndef _KERN_VMM_MPTCOPY_H_
#define _KERN_VMM_MPTCOPY_H_

#ifdef _KERN_

void copy_page_directory_structure(unsigned int pid_from, unsigned int pid_to);

#endif  /* _KERN_ */

#endif  /* !_KERN_VMM_MPTCOPY_H_ */
