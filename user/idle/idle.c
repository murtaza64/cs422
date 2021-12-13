#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>

int main(int argc, char **argv)
{
    printf("idle\n");

    for(unsigned int i = 0; 1; i++) {
        if (i % 10000000 == 0) {
            printf("idle says hi :D\n");
        }
    }
    return 0;
}
