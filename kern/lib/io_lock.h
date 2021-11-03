#ifndef _VDPRINTF_LOCK_
#define _VDPRINTF_LOCK_

#include <lib/io_lock.h>

void debug_spinlock_init(void);

void debug_lock(void);

void debug_unlock(void);

void input_spinlock_init(void);

void input_lock(void);

void input_unlock(void);

void output_spinlock_init(void);

void output_lock(void);

void output_unlock(void);


#endif