#include <lib/bbuf.h>

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/condvar.h>
// #define BBUF_MAX_SIZE 16

// typedef struct {
//     unsigned short front;
//     unsigned short back;
//     unsigned int contents[BBUF_MAX_SIZE];
//     unsigned int n_items;

//     spinlock_t lock;
//     condvar_t item_added;
//     condvar_t item_removed;
// } bbuf_t;

void bbuf_init(bbuf_t *bbuf) {
    bbuf->front = 0;
    bbuf->back = 0;

    spinlock_init(&(bbuf->lock));
    condvar_init(&(bbuf->item_added));
    condvar_init(&(bbuf->item_removed));
}

unsigned int bbuf_remove(bbuf_t *bbuf) {
    unsigned int item;
    
    spinlock_acquire(&(bbuf->lock));
    
    while (bbuf->n_items == 0) {
        condvar_wait(&(bbuf->item_added), &(bbuf->lock));
    }

    item = bbuf->contents[bbuf->front];
    bbuf->front = (bbuf->front + 1) % BBUF_MAX_SIZE;
    bbuf->n_items--;

    condvar_signal(&(bbuf->item_removed));
    spinlock_release(&(bbuf->lock));
    return item;
}

void bbuf_insert(bbuf_t *bbuf, unsigned int item) {
    spinlock_acquire(&(bbuf->lock));

    while (bbuf->n_items == BBUF_MAX_SIZE) {
        condvar_wait(&(bbuf->item_removed), &(bbuf->lock));
    }
    
    bbuf->contents[bbuf->back] = item;
    bbuf->back = (bbuf->back + 1) % BBUF_MAX_SIZE;
    bbuf->n_items++;

    condvar_signal(&(bbuf->item_added));
    spinlock_release(&(bbuf->lock));
}