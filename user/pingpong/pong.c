#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <sync.h>

extern unsigned int CLIENT_ID;

int main(int argc, char **argv)
{   
    unsigned int i, item;
    printf("pong started.\n");

    // testing mutex
    /*
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
    */

    bbuf *buf = (bbuf *) SHARED_PAGE_VADDR;
    CLIENT_ID = UNKNOWN_HOLDER;
    if ((CLIENT_ID = bbuf_init(buf)) == 0) {
        printf("[PONG #%d] inited buf :D\n", CLIENT_ID);
    }
    else {
        printf("[PONG #%d] i am a consumer :c\n", CLIENT_ID);
    }
    
    // full speed consuming
    for (i = 0; i < 8000000; i++) {
        if (i % 1 == 0) {
            item = bbuf_consume(buf);
            printf("<==== %d [PONG #%d] consume i=%d item=%d \n", item, CLIENT_ID, i/1, item);
        }
    }
}
