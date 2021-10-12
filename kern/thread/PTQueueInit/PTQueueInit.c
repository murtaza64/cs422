#include "lib/x86.h"

#include "import.h"
#include "lib/debug.h"

/**
 * Initializes all the thread queues with tqueue_init_at_id.
 */
void tqueue_init(unsigned int mbi_addr)
{
    // TODO: define your local variables here.
    unsigned int chid;

    tcb_init(mbi_addr);

    // TODO
    for (chid = 0; chid < NUM_IDS + 1; chid++) {
        tqueue_init_at_id(chid);
    }
}

/**
 * Insert the TCB #pid into the tail of the thread queue #chid.
 * Recall that the doubly linked list is index based.
 * So you only need to insert the index.
 * Hint: there are multiple cases in this function.
 */
void tqueue_enqueue(unsigned int chid, unsigned int pid)
{
    // TODO
    unsigned int tail_pid = tqueue_get_tail(chid);
    unsigned int head_pid = tqueue_get_head(chid);
    // First case: head is null (and tail)
    if (head_pid == NUM_IDS){ //degenerate case
        tcb_set_prev(pid, NUM_IDS);
        tcb_set_next(pid, NUM_IDS);
        tqueue_set_head(chid, pid);
        tqueue_set_tail(chid, pid);
    } else {
        tcb_set_prev(pid, tail_pid);
        tcb_set_next(pid, NUM_IDS);
        tcb_set_next(tail_pid, pid);
        tqueue_set_tail(chid, pid);
    }
}

/**
 * Reverse action of tqueue_enqueue, i.e. pops a TCB from the head of the specified queue.
 * It returns the popped thread's id, or NUM_IDS if the queue is empty.
 * Hint: there are multiple cases in this function.
 */
unsigned int tqueue_dequeue(unsigned int chid)
{
    // TODO
    unsigned int head_pid = tqueue_get_head(chid);
    unsigned int tail_pid = tqueue_get_tail(chid);
    //head is NUM_IDS (NULL)
    if (head_pid == NUM_IDS){ //empty case
        //do nothing
    } else if (head_pid == tail_pid) { //one item in queue
        tcb_set_prev(head_pid, NUM_IDS);
        tcb_set_next(head_pid, NUM_IDS);
        tqueue_set_head(chid, NUM_IDS);
        tqueue_set_tail(chid, NUM_IDS);
    } else { //regular case
        unsigned int next_pid = tcb_get_next(head_pid);
        tcb_set_prev(next_pid, NUM_IDS);
        tcb_set_next(head_pid, NUM_IDS);
        tqueue_set_head(chid, next_pid);
    }
    return head_pid;
}

/**
 * Removes the TCB #pid from the queue #chid.
 * Hint: there are many cases in this function.
 */
void tqueue_remove(unsigned int chid, unsigned int pid)
{
    // TODO
    unsigned int head_pid = tqueue_get_head(chid);
    unsigned int tail_pid = tqueue_get_tail(chid);
    unsigned int curr_pid = head_pid;
    unsigned int prev_pid = tcb_get_prev(curr_pid);
    unsigned int next_pid = tcb_get_next(curr_pid);

    // deals with empty case, one-elem case, and any case where pid is at head
    if (head_pid == pid || head_pid == NUM_IDS) {
        tqueue_dequeue(chid);
    } else if (tail_pid == pid) { // If TCB being removed is tail (and has precursor)
        unsigned int prev_pid = tcb_get_prev(tail_pid);
        tqueue_set_tail(chid, prev_pid);
        tcb_set_next(prev_pid, NUM_IDS);
        tcb_set_next(curr_pid, NUM_IDS);
        tcb_set_prev(curr_pid, NUM_IDS);
    } else while (curr_pid != NUM_IDS) {
        //regular case (precursor and successor)
        if (curr_pid == pid) {
            KERN_DEBUG("prev_pid=%d next_pid=%d\n", prev_pid, next_pid);
            tcb_set_next(prev_pid, next_pid);
            tcb_set_prev(next_pid, prev_pid);
            tcb_set_next(curr_pid, NUM_IDS);
            tcb_set_prev(curr_pid, NUM_IDS);
        }
        curr_pid = tcb_get_next(curr_pid);
        prev_pid = tcb_get_prev(curr_pid);
        next_pid = tcb_get_next(curr_pid);
    }
}
