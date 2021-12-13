#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include "sync.h"
#define VM_USERLO     0x40000000
#define PAGESIZE 4096
#define SHARED_PAGE_VADDR   VM_USERLO + 32 * PAGESIZE //???

int main(int argc, char **argv)
{
    printf("pong started.\n");
    // int *shared_data = SHARED_PAGE_VADDR;
    // *shared_data = 69;
    for(int i = 0; i < 20000; i++);
    // printf("pong: shared data: %d\n", *shared_data);
    while (1) {
        yield();
    }
    return 0;
}
