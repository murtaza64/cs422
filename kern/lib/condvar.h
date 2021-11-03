#ifndef _KERN_LIB_CONDVAR_H_
#define _KERN_LIB_CONDVAR_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>

typedef struct {
    unsigned short front;
    unsigned short back;
    unsigned int pid_array[NUM_IDS];
    unsigned int n_pids;
} cvqueue_t;

typedef struct {
    cvqueue_t queue;
    spinlock_t lock;
} condvar_t;


void condvar_init(condvar_t *cv);

void condvar_wait(condvar_t *cv, spinlock_t *lock);
void condvar_signal(condvar_t *cv);
void condvar_broadcast(condvar_t *cv);

void cvqueue_init(cvqueue_t *cvq);
unsigned int cvqueue_is_empty(cvqueue_t *cvq);
unsigned int cvqueue_enqueue(cvqueue_t *cvq, unsigned int pid);
unsigned int cvqueue_dequeue(cvqueue_t *cvq);

#endif
#endif