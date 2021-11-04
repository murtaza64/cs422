#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <thread/PCurID/export.h>
#include <lib/condvar.h>
#include <lib/debug.h>
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


extern spinlock_t thread_lock;

void condvar_init(condvar_t *cv) {
    cvqueue_init(&(cv->queue));
}

void condvar_wait(condvar_t *cv, spinlock_t *lock) {
    unsigned int pid = get_curid();
    intr_local_disable();
    //TODO: assert lock held by current thread (kern panic?)
    cvqueue_enqueue(&(cv->queue), pid); //TODO: add error state for full enqueue?
    thread_cv_suspend(lock);
    intr_local_enable();
    spinlock_acquire(lock);

    #ifdef SHOW_LOCKING
    intr_local_disable();
    KERN_DEBUG("[BBUF] wait (cpu %d pid %d): reaquired bbuf lock\n", get_pcpu_idx(), get_curid());
    intr_local_enable();
    #endif
}

void condvar_signal(condvar_t *cv) {
    if (!cvqueue_is_empty(&(cv->queue))){
        intr_local_disable();
        threadinfo_t t = cvqueue_dequeue(&(cv->queue));

        spinlock_acquire(&thread_lock);
        #ifdef SHOW_LOCKING
        KERN_DEBUG("condvar_signal: found waiting thread %d (was on cpu %d)\n", t.pid, t.cpu_idx);
        #endif
        //TODO: add error state for empty dequeue?
        tcb_set_state(t.pid, TSTATE_READY);
        //enqueue to same cpu ready queue as originally found, not current cpu
        tqueue_enqueue(NUM_IDS + t.cpu_idx, t.pid); 
        spinlock_release(&thread_lock);

        #ifdef SHOW_LOCKING
        KERN_DEBUG("condvar_signal: complete (pid %d)\n", get_curid());
        #endif
        
        intr_local_enable();
    }
}

void condvar_broadcast(condvar_t *cv) {
    while (!cvqueue_is_empty(&(cv->queue))){
        intr_local_disable();
        spinlock_acquire(&thread_lock);
        threadinfo_t t = cvqueue_dequeue(&(cv->queue));
        //TODO: add error state for empty dequeue?
        tcb_set_state(t.pid, TSTATE_READY);
        tqueue_enqueue(NUM_IDS + t.cpu_idx, t.pid);
        spinlock_release(&thread_lock);
        intr_local_enable();
    }
}

void cvqueue_init(cvqueue_t *cvq) {
    cvq->front = 0;
    cvq->back = 0;
    cvq->n_items = 0;
}

unsigned int cvqueue_is_empty(cvqueue_t *cvq) {
    return cvq->n_items == 0;
}
//returns 0 if enqueue was unsuccessful
unsigned int cvqueue_enqueue(cvqueue_t *cvq, unsigned int pid) {
    if (cvq->n_items == NUM_IDS) {
        // #ifdef SHOW_LOCKING
        KERN_PANIC("enqueued to full cvq!");
        // #endif
        return 0;
    }
    cvq->thread_array[cvq->back].pid = pid;
    cvq->thread_array[cvq->back].cpu_idx = get_pcpu_idx();
    cvq->back = (cvq->back + 1) % NUM_IDS;
    cvq->n_items++;
    return 1;
}
//returns {NUM_IDS, NUM_CPUS} if queue is empty
threadinfo_t cvqueue_dequeue(cvqueue_t *cvq) {
    if (cvq->n_items == 0) {
        KERN_PANIC("dequeued from empty cvq!");
        // return (threadinfo_t) {NUM_IDS, NUM_CPUS};
        //TODO: kernel panic?
    }
    threadinfo_t t = cvq->thread_array[cvq->front];
    cvq->front = (cvq->front + 1) % NUM_IDS;
    cvq->n_items--;
    return t;
}