#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include "sync.h"
#define VM_USERLO     0x40000000
#define PAGESIZE 4096
#define SHARED_PAGE_VADDR   VM_USERLO + (32 * PAGESIZE) //???

int main(int argc, char **argv)
{
    printf("ping started.\n");
    printf("0x%x\n", SHARED_PAGE_VADDR);
    // int *shared_data = SHARED_PAGE_VADDR;
    // *shared_data = 69;
    // printf("ping: shared data at 0x%x: %d\n", shared_data, *shared_data);
    while (1) {
        yield();
    }
    return 0;
}
