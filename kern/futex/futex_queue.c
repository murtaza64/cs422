#include <lib/x86.h>
#include <thread/PTCBIntro/export.h>
#include "futex_queue.h"

void futexq_enqueue(struct futexq *q, unsigned int pid)
{
    uint32_t tail = q->tail;

    if (tail == NUM_IDS) {
        tcb_set_prev(pid, NUM_IDS);
        tcb_set_next(pid, NUM_IDS);
        q->head = q->tail = pid;
    } else {
        tcb_set_next(tail, pid);
        tcb_set_prev(pid, tail);
        tcb_set_next(pid, NUM_IDS);
        q->tail = pid;
    }
}


uint32_t futexq_dequeue(struct futexq *q)
{
    uint32_t head, next, pid;

    pid = NUM_IDS;
    head = q->head;

    if (head != NUM_IDS) {
        pid = head;
        next = tcb_get_next(head);

        if (next == NUM_IDS) {
            q->head = q->tail = NUM_IDS;
        } else {
            tcb_set_prev(next, NUM_IDS);
            q->head = next;
        }
        tcb_set_prev(pid, NUM_IDS);
        tcb_set_next(pid, NUM_IDS);
    }

    return pid;
}