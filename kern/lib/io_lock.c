#include <lib/gcc.h>
#include <lib/spinlock.h>

#include <lib/io_lock.h>

static spinlock_t input_lk;
static spinlock_t output_lk;

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

void output_lock(void) {
    spinlock_acquire(&output_lk);
}

void output_unlock(void) {
    spinlock_release(&output_lk);
}
