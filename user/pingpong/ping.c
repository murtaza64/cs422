#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("ping started.\n");

    // fast producing
    for (i = 0; i < 0; i++)
        produce();

    // slow producing
    for (i = 0; i < 8000000; i++) {
        if (i % 100000 == 0) {
            printf("produce %d \n", i/100000);
            produce();
        }
    }

    return 0;
}
