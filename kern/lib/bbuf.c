#include <lib/bbuf.h>

#include <lib/gcc.h>
#include <lib/types.h>
#include <lib/x86.h>
#include <lib/spinlock.h>
#include <lib/condvar.h>
#include <lib/debug.h>
#include <thread/PCurID/export.h>
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

extern int dprintf(const char *fmt, ...);

bbuf_t bbuf;

void bbuf_init(bbuf_t *bbuf) {
    bbuf->front = 0;
    bbuf->back = 0;
    bbuf->n_items = 0;

    spinlock_init(&(bbuf->lock));
    condvar_init(&(bbuf->item_added));
    condvar_init(&(bbuf->item_removed));
}


void bbuf_init_wrapper() {
    bbuf_init(&bbuf);
}

unsigned int bbuf_remove(bbuf_t *bbuf) {
    unsigned int item;
    #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] thread %d acquiring bbuf spinlock... \n", get_curid());
    intr_local_enable();
    #endif
    spinlock_acquire(&(bbuf->lock));
    #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] thread %d acquired bbuf spinlock\n", get_curid());
    intr_local_enable();
    #endif
    while (bbuf->n_items == 0) {
        intr_local_disable();
        dprintf("[BBUF] thread %d waiting on item_added (items=%d)\n", get_curid(), bbuf->n_items);
        intr_local_enable();
        condvar_wait(&(bbuf->item_added), &(bbuf->lock));
    }

    item = bbuf->contents[bbuf->front];
    bbuf->front = (bbuf->front + 1) % BBUF_MAX_SIZE;
    bbuf->n_items--;
    // #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] bbuf_remove: %d items\n", bbuf->n_items);
    intr_local_enable();
    // #endif

    condvar_signal(&(bbuf->item_removed));
    spinlock_release(&(bbuf->lock));

    #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] thread %d released bbuf spinlock\n", get_curid());
    intr_local_enable();
    #endif

    return item;
}

void bbuf_insert(bbuf_t *bbuf, unsigned int item) {
    #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] thread %d acquiring bbuf spinlock... \n", get_curid());
    intr_local_enable();
    #endif

    spinlock_acquire(&(bbuf->lock));
    
    #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] thread %d acquired bbuf spinlock\n", get_curid());
    intr_local_enable();
    #endif

    while (bbuf->n_items == BBUF_MAX_SIZE) {
        // #ifdef SHOW_LOCKING
        intr_local_disable();
        dprintf("[BBUF] thread %d waiting on item_removed\n", get_curid());
        intr_local_enable();
        // #endif

        condvar_wait(&(bbuf->item_removed), &(bbuf->lock));
    }
    
    bbuf->contents[bbuf->back] = item;
    bbuf->back = (bbuf->back + 1) % BBUF_MAX_SIZE;
    bbuf->n_items++;
    // #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] bbuf_insert: %d items\n", bbuf->n_items);
    intr_local_enable();
    // #endif

    condvar_signal(&(bbuf->item_added));
    spinlock_release(&(bbuf->lock));

    #ifdef SHOW_LOCKING
    intr_local_disable();
    dprintf("[BBUF] thread %d released bbuf spinlock\n", get_curid());
    intr_local_enable();
    #endif
}