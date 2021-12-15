#include <stdio.h>
#define VM_USERLO     0x40000000
#define VM_USERHI     0xF0000000
#define PAGESIZE 4096
//???
#define SHARED_PAGE_VADDR   VM_USERLO + (32 * PAGESIZE)

int main() {
    unsigned int x = 0x40020000;
    void *y = (void *) SHARED_PAGE_VADDR;
    int *z = (int *) (SHARED_PAGE_VADDR);
    printf("%x %x %x\n", x, y, z);
}