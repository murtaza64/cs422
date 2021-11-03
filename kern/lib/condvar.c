#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <thread/PCurID/export.h>
#include <lib/condvar.h>
#include <thread/PTQueueInit/export.h>
#include <pcpu/PCPUIntro/export.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PThread/export.h>
#include <lib/thread.h>

// typedef struct {
//     unsigned short front;
//     unsigned short back;
//     unsigned int pid_array[NUM_IDS];
// } cvqueue_t;

// typedef struct {
//     cvqueue_t queue;
//     spinlock_t lock;
// } condvar_t;


void condvar_init(condvar_t *cv) {
    cvqueue_init(&(cv->queue));
    // spinlock_init(&(cv->lock));
}

void condvar_wait(condvar_t *cv, spinlock_t *lock) {
    // spinlock_acquire(&(cv->lock));
    cvqueue_enqueue(&(cv->queue), get_curid());
    // spinlock_release(&(cv->lock));
    thread_cv_yield(lock);
}

void condvar_signal(condvar_t *cv) {
    if (!cvqueue_is_empty(&(cv->queue))){
        unsigned int pid = cvqueue_dequeue(&(cv->queue));
        tcb_set_state(pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS + get_pcpu_idx(), pid);
    }
}

void condvar_broadcast(condvar_t *cv) {
    while (!cvqueue_is_empty(&(cv->queue))){
        unsigned int pid = cvqueue_dequeue(&(cv->queue));
        tcb_set_state(pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS + get_pcpu_idx(), pid);
    }
}

void cvqueue_init(cvqueue_t *cvq) {
    cvq->front = 0;
    cvq->back = 0;
    cvq->n_pids = NUM_IDS;
    for (int i = 0; i < NUM_IDS; i++) {
        cvq->pid_array[0] = NUM_IDS;
    }
}

unsigned int cvqueue_is_empty(cvqueue_t *cvq) {
    return cvq->n_pids == 0;
}
//returns 0 if enqueue was unsuccessful
unsigned int cvqueue_enqueue(cvqueue_t *cvq, unsigned int pid) {
    if (cvq->n_pids == NUM_IDS) {
        return NUM_IDS;
    }
    cvq->pid_array[cvq->back] = pid;
    cvq->back = (cvq->back + 1) % NUM_IDS;
    cvq->n_pids++;
    return pid;
}
//return NUM_IDS if queue is empty
unsigned int cvqueue_dequeue(cvqueue_t *cvq) {
    if (cvq->n_pids == 0) {
        return NUM_IDS;
    }
    unsigned int return_pid = cvq->pid_array[cvq->front];
    cvq->pid_array[cvq->front] = NUM_IDS;
    cvq->front = (cvq->front + 1) % NUM_IDS;
    cvq->n_pids--;
    return return_pid;
}