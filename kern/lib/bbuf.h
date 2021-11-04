#ifndef _KERN_LIB_BBUF_H_
#define _KERN_LIB_BBUF_H_

#ifdef _KERN_

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/condvar.h>
#define BBUF_MAX_SIZE 16

typedef struct {
    unsigned short front;
    unsigned short back;
    unsigned int contents[BBUF_MAX_SIZE];
    unsigned int n_items;

    spinlock_t lock;
    condvar_t item_added;
    condvar_t item_removed;
} bbuf_t;

void bbuf_init(bbuf_t *bbuf);
void bbuf_init_wrapper();
unsigned int bbuf_remove(bbuf_t *bbuf);
void bbuf_insert(bbuf_t *bbuf, unsigned int item);

#endif
#endif