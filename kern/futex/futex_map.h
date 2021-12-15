#ifndef _KERN_FUTEX_MAP_H_
#define _KERN_FUTEX_MAP_H_
#include <kern/lib/types.h>

#define FUTEX_POOL_SIZE 128
#define NUM_IDS 64

// Function to get futex pointer from map
uint32_t hash_key (uint32_t key);
// Function to get futex pointer from map
struct futexq *futex_map_get(uint32_t key);
// finds a spot in the map for key
struct futexq *futex_map_create(uint32_t key);
// returns a pointer to the futex wait queue for paddr
struct futexq *futex_map_get_or_create(uint32_t *paddr);
// Function to remove the pointer at a specific key
void futex_map_delete (uint32_t key);

#endif