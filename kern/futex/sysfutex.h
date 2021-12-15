#ifndef _KERN_SYS_FUTEX_H_
#define _KERN_SYS_FUTEX_H_

void sys_futex_wait(tf_t *tf);
void sys_futex_wake(tf_t *tf);
void sys_futex_cmp_requeue(tf_t *tf);


#endif