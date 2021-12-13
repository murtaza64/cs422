#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <sync.h>
#define VM_USERLO     0x40000000
#define PAGESIZE 4096
#define SHARED_PAGE_VADDR   VM_USERLO + (32 * PAGESIZE) //???

int main(int argc, char **argv)
{
    printf("[PING] ping started.\n");
    printf("0x%x\n", SHARED_PAGE_VADDR);
    mutex *m = SHARED_PAGE_VADDR;

    mutex_init(m);

    printf("[PING] acquiring lock...\n");
    mutex_acquire(m);
    printf("[PING] I, ping, have the lock! %x\n", &m->lock);
    
    for (int i = 0; i < 100000000; i++) {
        if (i%20000000 == 0) {
            printf("[PING] doing stuff... %x\n", &m->lock);
        }
    }
    mutex_release(m);
    printf("[PING] I, ping, am done with the lock! %x\n", &m->lock);
    while (1) {
        yield();
    }
    return 0;
}
