#include <lib/x86.h>
#include <kern/lib/debug.h>
#include "futex_queue.h"
#include "futex_map.h"

struct futexq futexq_pool[FUTEX_POOL_SIZE];

uint32_t hash_key (uint32_t key) {
    return (key * 2654435761) % FUTEX_POOL_SIZE;
}

// Function to get futex pointer from map
// returns NULL if not found in map
struct futexq *futex_map_get(uint32_t key) {
    int index = hash_key(key);
    int initial_index = index;

    do {
        if (futexq_pool[index].paddr == key) {
            return &futexq_pool[index];
        }

        //go to next
        ++index;
        index %= FUTEX_POOL_SIZE;
    } while (index != initial_index && futexq_pool[index].paddr != 0);

    return NULL;
}

// find a spot in the map for futexq for key
struct futexq *futex_map_create(uint32_t key) {
    uint32_t index = hash_key(key);
    uint32_t initial_index = index; 
    
    do {
        //queue is empty
        if (futexq_pool[index].head == NUM_IDS) {
            futexq_pool[index].paddr = key;
            return &futexq_pool[index];
        }
        ++index;
        index %= 128;
    } while(index != initial_index);
    KERN_PANIC("no space left in futex map"); //TODO
    return NULL;
}

//returns a pointer to the futex wait queue for paddr
struct futexq *futex_map_get_or_create(uint32_t *paddr) {
    struct futexq *q;
    if ((q = futex_map_get((uint32_t) paddr)) != NULL) {
        return futex_map_create((uint32_t) paddr);
    }
    KERN_PANIC("cannot create or get futex"); //TODO
    return NULL;
}

// Function to remove the pointer at a specific key
void futex_map_delete (uint32_t key) {
    // :)
}