#include <sync.h>
#include <syscall.h>
#include <stdio.h>

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
    while (!mutex_try_acquire(m)) {
        printf("[SYNC] instant lock acquire failed. calling futex_wait...\n");
        sys_futex_wait(&m->lock, 1);
    }
    printf("[SYNC] lock acquired!\n");
}

void mutex_release(mutex* m) {
    xchg(&m->lock, 0);
    sys_futex_wake(&m->lock, 1);
}

void mutex_init(mutex* m) {
    //TODO does this work?
    if (xchg(&m->inited, SYNC_MAGIC_NUMBER) != SYNC_MAGIC_NUMBER) {
        if (xchg(&m->lock, 1) != 1) {
            m->lock = 0;
        }
    }
}