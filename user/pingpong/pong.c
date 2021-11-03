#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("pong started.\n");

    for (i = 0; i < 8000000; i++) {
        if (i % 100000 == 0) {
            printf("consume %d \n", i/100000);
            consume();
        }
    }

    return 0;
}
