#include <proc.h>
#include <stdio.h>
#include <syscall.h>

int main(int argc, char **argv)
{
    unsigned int i;
    printf("pong started.\n");

    for (i = 0; i < 8000; i++) {
        if (i % 130 == 0)
            consume();
    }

    return 0;
}
