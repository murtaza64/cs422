#include <lib/gcc.h>
#include <lib/spinlock.h>

#include <lib/io_lock.h>

static spinlock_t input_lk;
static spinlock_t output_lk;
static spinlock_t debug_lk;

void debug_spinlock_init(void) {
    spinlock_init(&debug_lk);
}

void debug_lock(void) {
    spinlock_acquire(&debug_lk);
}

void debug_unlock(void) {
    spinlock_release(&debug_lk);
}

void input_spinlock_init(void) {
    spinlock_init(&input_lk);
}

void input_lock(void) {
    spinlock_acquire(&input_lk);
}

void input_unlock(void) {
    spinlock_release(&input_lk);
}

void output_spinlock_init(void) {
    spinlock_init(&output_lk);
}

int output_lock(void) {
    if (!spinlock_holding(&output_lk)) {
        spinlock_acquire(&output_lk);
        return 1;
    }
    return 0;
}

void output_unlock(void) {
    spinlock_release(&output_lk);
}
