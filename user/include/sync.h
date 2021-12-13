#ifndef _USER_SYNC_H_
#define _USER_SYNC_H_

#define VM_USERLO     0x40000000
#define VM_USERHI     0xF0000000
#define VM_USERLO_PDE (VM_USERLO / PDIRSIZE)
#define VM_USERHI_PDE (VM_USERHI / PDIRSIZE)
//???
#define SHARED_PAGE_VADDR   VM_USERLO + (32 * PAGESIZE)
#define BBUF_SIZE 16
#define SYNC_MAGIC_NUMBER 0xdeadbeef

typedef unsigned int uint32_t;

typedef struct {
    unsigned int lock;
    int holder;
    int inited;
} mutex;

unsigned int mutex_try_acquire(mutex* m);
void mutex_acquire(mutex* m);
void mutex_release(mutex* m);

typedef struct {
    int val;
} condvar;

void cv_signal(condvar* cv);
void cv_wait(condvar* cv, mutex* mutex);
void cv_broadcast(condvar* cv);

typedef struct {
    int buf[BBUF_SIZE];
    unsigned int n;
    condvar not_empty;
    condvar not_full;
    mutex mutex;
    unsigned int inited;
    unsigned int clients_registered;
} bbuf;

unsigned int bbuf_init(bbuf* bbuf);
void bbuf_produce(bbuf* bbuf, int item);
int bbuf_consume(bbuf* bbuf);

#endif