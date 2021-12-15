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
    if (xchg(&m->lock, LOCK_HELD) == LOCK_FREE) {
        m->holder = CLIENT_ID;
        return TRUE;
    }
    return FALSE;
}

void mutex_acquire(mutex* m) {
    while (!(m->inited == SYNC_MAGIC_NUMBER)) {}
    while (!mutex_try_acquire(m)) {
        printf("[SYNC #%d] %x instant lock acquire failed (holder = %d). calling futex_wait...\n", CLIENT_ID, m, m->holder);
        sys_futex_wait(&m->lock, LOCK_HELD);
    }
    printf("[SYNC #%d] lock %x acquired!\n", CLIENT_ID, m);
}

void mutex_release(mutex* m) {
    m->holder = NO_HOLDER;
    xchg(&m->lock, 0);
    printf("[SYNC #%d] releasing %x...\n", CLIENT_ID, m);
    sys_futex_wake(&m->lock, 1);
    printf("[SYNC #%d] lock %x released\n", CLIENT_ID, m);
}

//initialize and acquire lock
void mutex_init(mutex* m) {
    //TODO does this work?
    if (xchg(&m->lock, LOCK_HELD) != LOCK_HELD) {
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

unsigned int bbuf_init(bbuf *buf) {
    if (xchg(&buf->inited, SYNC_MAGIC_NUMBER) == SYNC_MAGIC_NUMBER) {
        mutex_acquire(&buf->mutex);
        unsigned int ret = buf->clients_registered++;
        mutex_release(&buf->mutex);
        return ret;
    }
    printf("[BBUF INIT] &mutex = 0x%x &not_full.val = 0x%x &not_empty.val 0x%x\n", 
            &buf->mutex, &buf->not_full.val, &buf->not_empty.val);
    mutex_init(&buf->mutex); //this also acquires the lock
    buf->n = 0;
    buf->head = buf->tail = 0;
    buf->not_empty.val = FALSE;
    buf->not_full.val = TRUE;
    buf->clients_registered = 1;
    mutex_release(&buf->mutex);
    return 0;
}

void bbuf_produce(bbuf *buf, int item) {
    mutex_acquire(&buf->mutex);
    while (buf->n == BBUF_SIZE) {
        printf("[PRODUCE #%d] waiting for not full...\n", CLIENT_ID);
        cv_wait(&buf->not_full, &buf->mutex);
    }
    buf->buf[buf->tail] = item;
    buf->tail = (buf->tail + 1) % BBUF_SIZE;
    buf->n++;
    cv_signal(&buf->not_empty);
    printf("[PRODUCE #%d] produced %d, buf->n = %d\n", CLIENT_ID, item, buf->n);
    mutex_release(&buf->mutex);
}

int bbuf_consume(bbuf *buf) {
    int consumed;
    mutex_acquire(&buf->mutex);
    while (buf->n == 0) {
        printf("[CONSUME #%d] waiting for not empty...\n", CLIENT_ID);
        cv_wait(&buf->not_empty, &buf->mutex);
    }
    consumed = buf->buf[buf->head];
    buf->head = (buf->head + 1) % BBUF_SIZE;
    buf->n--;
    // printf("[CONSUME #%d] updated bbuf %d %d\n", CLIENT_ID, consumed, buf->n);
    cv_signal(&buf->not_full);
    printf("[CONSUME #%d] consumed %d, buf->n = %d\n", CLIENT_ID, consumed, buf->n);
    mutex_release(&buf->mutex);
    return consumed;
}