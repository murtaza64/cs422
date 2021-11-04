#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    unsigned int item;
    printf("pong started.\n");

    for (i = 0; i < 8000000; i++) {
        if (i % 1 == 0) {
            item = consume();
            printf("[USER] consume i=%d item=%d \n", i/1, item);
        }
    }

    return 0;
}
