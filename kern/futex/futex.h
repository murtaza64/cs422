#ifndef _KERN_FUTEX_H_
#define _KERN_FUTEX_H_
#include <kern/lib/types.h>



void futex_wait(uint32_t *paddr, uint32_t val);
uint32_t futex_wake(uint32_t *paddr, uint32_t num_wake);
uint32_t futex_cmp_requeue(uint32_t paddr, uint32_t cmpval, uint32_t *dest_paddr, uint32_t n_wake, uint32_t n_move);

#endif