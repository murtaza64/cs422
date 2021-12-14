#include <sync.h>
#include <syscall.h>
#include <stdio.h>
#include <types.h>

typedef unsigned int uint32_t;
unsigned int CLIENT_ID;

static inline uint32_t xchg(volatile uint32_t *addr, uint32_t newval)
{
    uint32_t result;

    asm volatile ("lock; xchgl %0, %1"
                  : "+m" (*addr), "=a" (result)
                  : "1" (newval)
                  : "cc");
    return result;
}

unsigned int mutex_try_acquire(mutex* m) {
    uint32_t old_val = xchg(&m->lock, 1);
    if (old_val == 0) {
        m->holder = CLIENT_ID;
    }
    return old_val == 0;
}

void mutex_acquire(mutex* m) {
    while (!(m->inited == SYNC_MAGIC_NUMBER)) {}
    while (!mutex_try_acquire(m)) {
        printf("[SYNC %d] %x instant lock acquire failed (holder = %d). calling futex_wait...\n", CLIENT_ID, m, m->holder);
        sys_futex_wait(&m->lock, 1);
    }
    printf("[SYNC %d] lock %x acquired!\n", CLIENT_ID, m);
}

void mutex_release(mutex* m) {
    m->holder = NO_HOLDER;
    xchg(&m->lock, 0);
    printf("[SYNC %d] releasing %x...\n", CLIENT_ID, m);
    sys_futex_wake(&m->lock, 1);
    printf("[SYNC %d] lock %x released\n", CLIENT_ID, m);
}

//initialize and acquire lock
void mutex_init(mutex* m) {
    //TODO does this work?
    if (xchg(&m->lock, 1) != 1) {
        if (xchg(&m->inited, SYNC_MAGIC_NUMBER) != SYNC_MAGIC_NUMBER) {
            m->holder = CLIENT_ID;
            // m->lock = 0;
        }
    }
}

void cv_signal(condvar *cv) {
    cv->val = TRUE;
    sys_futex_wake(&cv->val, 1);
}

void cv_wait(condvar *cv, mutex *m) {
    cv->val = FALSE;
    mutex_release(m);
    sys_futex_wait(&cv->val, FALSE);
    mutex_acquire(m);
}

unsigned int bbuf_init(bbuf *bbuf) {
    if (xchg(&bbuf->inited, SYNC_MAGIC_NUMBER) == SYNC_MAGIC_NUMBER) {
        mutex_acquire(&bbuf->mutex);
        unsigned int ret = bbuf->clients_registered++;
        mutex_release(&bbuf->mutex);
        return ret;
    }
    printf("[BBUF INIT] &mutex = 0x%x &not_full.val = 0x%x &not_empty.val 0x%x\n", 
            &bbuf->mutex, &bbuf->not_full.val, &bbuf->not_empty.val);
    mutex_init(&bbuf->mutex);
    // mutex_acquire(&bbuf->mutex);
    bbuf->n = 0;
    bbuf->head = bbuf->tail = 0;
    bbuf->not_empty.val = FALSE;
    bbuf->not_full.val = TRUE;
    bbuf->clients_registered = 1;
    mutex_release(&bbuf->mutex);
    return 0;
}

void bbuf_produce(bbuf* bbuf, int item) {
    mutex_acquire(&bbuf->mutex);
    while (bbuf->n == BBUF_SIZE) {
        printf("[PRODUCE %d] waiting for not full...\n", CLIENT_ID);
        cv_wait(&bbuf->not_full, &bbuf->mutex);
    }
    bbuf->buf[bbuf->tail] = item;
    bbuf->tail = (bbuf->tail + 1) % BBUF_SIZE;
    bbuf->n++;

    cv_signal(&bbuf->not_empty);
    mutex_release(&bbuf->mutex);
}

int bbuf_consume(bbuf* bbuf) {
    int consumed;
    mutex_acquire(&bbuf->mutex);
    while (bbuf->n == 0) {
        printf("[CONSUME %d] waiting for not empty...\n", CLIENT_ID);
        cv_wait(&bbuf->not_empty, &bbuf->mutex);
    }
    consumed = bbuf->buf[bbuf->head];
    bbuf->head = (bbuf->head + 1) % BBUF_SIZE;
    bbuf->n--;
    printf("[CONSUME %d] updated bbuf %d %d\n", CLIENT_ID, consumed, bbuf->n);
    cv_signal(&bbuf->not_full);
    printf("[CONSUME %d] signal finished (consumed = %d)\n", CLIENT_ID, consumed);
    mutex_release(&bbuf->mutex);
    return consumed;
}