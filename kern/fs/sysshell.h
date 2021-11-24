// system calls for shell

#ifndef _KERN_FS_SYSSHELL_H_
#define _KERN_FS_SYSSHELL_H_

#ifdef _KERN_

void sys_ls(tf_t *tf);
void sys_pwd(tf_t *tf);
void sys_isdir(tf_t *tf);
void sys_cp_recursive(tf_t *tf);
void sys_rm_recursive(tf_t *tf);
void sys_mv_dir(tf_t *tf);

#endif  /* _KERN_ */

#endif  /* !_KERN_FS_SYSSHELL_H_ */
