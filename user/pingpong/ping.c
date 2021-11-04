#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    unsigned int item;
    printf("[USER] ping started.\n");

    // fast producing
    // for (i = 0; i < 0; i++)
    //     produce();

    // slow producing
    for (i = 0; i < 8000000; i++) {
        if (i % 1 == 0) {
            item = i*3+1;
            printf("[USER] produce i=%d item=%d \n", i/1, item);
            produce(item);
        }
    }

    return 0;
}
