#include <lib/x86.h>
#include <lib/thread.h>
#include <lib/debug.h>
#include "import.h"

void thread_init(unsigned int mbi_addr)
{
    tqueue_init(mbi_addr);
    set_curid(0);
    tcb_set_state(0, TSTATE_RUN);
}

/**
 * Allocates a new child thread context, sets the state of the new child thread
 * to ready, and pushes it to the ready queue.
 * It returns the child thread id.
 */
unsigned int thread_spawn(void *entry, unsigned int id, unsigned int quota)
{
    // TODO
    unsigned int child_pid = kctx_new(entry, id, quota);
    // TSTATE_READY = 0
    if (child_pid != NUM_IDS) {
        tcb_set_state(child_pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS, child_pid);
    }
    return child_pid;
}

/**
 * Yield to the next thread in the ready queue.
 * You should set the currently running thread state as ready,
 * and push it back to the ready queue.
 * Then set the state of the popped thread as running, set the
 * current thread id, and switch to the new kernel context.
 * Hint: If you are the only thread that is ready to run,
 * do you need to switch to yourself?
 */
void thread_yield(void)
{
    // TODO
    unsigned int curid = get_curid();
    unsigned int nextid = tqueue_dequeue(NUM_IDS);
    if (nextid != NUM_IDS) {
        // KERN_DEBUG("entering thread yield from pid %d to %d\n", curid, nextid);
        tcb_set_state(curid, TSTATE_READY);
        if(curid != 1) {
            tqueue_enqueue(NUM_IDS, curid);
        }
        tcb_set_state(nextid, TSTATE_RUN);
        set_curid(nextid);
        kctx_switch(curid, nextid);
        // KERN_DEBUG("got to the end of thread yield from pid %d to %d\n", curid, nextid);
        set_curid(curid);
    }
}
