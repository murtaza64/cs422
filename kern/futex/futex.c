#include "futex.h"
#include "futex_map.h"
#include "futex_queue.h"
#include <kern/thread/PCurID/export.h>



void futex_wait(uint32_t *paddr, uint32_t val) {
    if (*paddr != val) {
        //return EAGAIN
    }
    struct futexq *q = futex_map_get_or_create(paddr);
    futexq_enqueue(q, get_curid());
}

