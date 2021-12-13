#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <sync.h>
#define VM_USERLO     0x40000000
#define PAGESIZE 4096
#define SHARED_PAGE_VADDR   VM_USERLO + (32 * PAGESIZE) //???

int main(int argc, char **argv)
{
    printf("pong started.\n");
    mutex *m = SHARED_PAGE_VADDR;

    for(int i = 0; i < 2000000; i++);
    mutex_init(m);

    printf("[PONG] acquiring lock...\n");
    mutex_acquire(m);
    printf("I, pong, have the lock! %x\n", &m->lock);
    
    for (int i = 0; i < 1000000; i++);
    mutex_release(m);
    printf("I, pong, am done with the lock! %x\n", &m->lock);

    while (1) {
        yield();
    }
    return 0;
}
