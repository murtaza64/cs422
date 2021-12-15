#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <sync.h>

extern unsigned int CLIENT_ID;
// int primes[6] = {2, 3, 5, 7, 11, 13};

int main(int argc, char **argv)
{   
    unsigned int i, item;
    printf("[PING] ping started.\n");
    
    // the below tests the basic operation of mutex
    /*
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
    */

    // bbuf *buf = SHARED_PAGE_VADDR;
    bbuf *buf = (bbuf *) SHARED_PAGE_VADDR;
    printf("%x\n", buf);
    CLIENT_ID = UNKNOWN_HOLDER;
    if ((CLIENT_ID = bbuf_init(buf)) == 0) {
        printf("[PING #%d] inited buf :D\n", CLIENT_ID);
    }
    else {
        printf("[PING #%d] i am a producer :p\n", CLIENT_ID);
    }
    
    // full speed producing
    for (i = 0; i < 8000000; i++) {
        if (i % 1 == 0) {
            item = i*3+1;
            printf("%d =====> [PING #%d] produce i=%d item=%d \n", item, CLIENT_ID, i/1, item);
            bbuf_produce(buf, item);
        }
    }
    return 0;
}
