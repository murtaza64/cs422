#ifndef _KERN_FUTEX_QUEUE_H_
#define _KERN_FUTEX_QUEUE_H_

#include <kern/lib/spinlock.h>

struct futexq {
    unsigned int paddr;
    spinlock_t lock;
    unsigned int head;
    unsigned int tail;
};

// unsigned int futexq_get_head(unsigned int paddr);
// void futexq_set_head(unsigned int paddr, unsigned int head);
// unsigned int futexq_get_tail(unsigned int paddr);
// void futexq_set_tail(unsigned int paddr, unsigned int tail);
// void futexq_init_at_paddr(unsigned int paddr);

// enqueues pid onto q
void futexq_enqueue(struct futexq *q, uint32_t pid);

// dequeue and return pid dequeued or NUM_IDS for failure
uint32_t futexq_dequeue(struct futexq *q);

// void futexq_remove(unsigned int chid, unsigned int pid);

#endif