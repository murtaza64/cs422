#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <sync.h>
#define VM_USERLO     0x40000000
#define PAGESIZE 4096
#define SHARED_PAGE_VADDR   VM_USERLO + (32 * PAGESIZE) //???

extern unsigned int CLIENT_ID;

int main(int argc, char **argv)
{   
    unsigned int i, item;
    printf("pong started.\n");
    // mutex *m = SHARED_PAGE_VADDR;

    // for(int i = 0; i < 2000000; i++);
    // mutex_init(m);

    // printf("[PONG] acquiring lock...\n");
    // mutex_acquire(m);
    // printf("I, pong, have the lock! %x\n", &m->lock);
    
    // for (int i = 0; i < 1000000; i++);
    // mutex_release(m);
    // printf("I, pong, am done with the lock! %x\n", &m->lock);

    // while (1) {
    //     yield();
    // }
    // return 0;

    bbuf *bbuf = SHARED_PAGE_VADDR;
    CLIENT_ID = UNKNOWN_HOLDER;
    if ((CLIENT_ID = bbuf_init(bbuf)) == 0) {
        printf("[PONG %d] inited bbuf :D\n", CLIENT_ID);
    }
    else {
        printf("[PONG %d] i am a consumer :c\n", CLIENT_ID);
    }
    
    // slow producing
    for (i = 0; i < 8000000; i++) {
        if (i % 1 == 0) {
            item = bbuf_consume(bbuf);
            printf("<==== %d [PONG %d] consume i=%d item=%d \n", item, CLIENT_ID, i/1, item);
        }
    }
}
