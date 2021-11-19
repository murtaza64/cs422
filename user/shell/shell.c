#include <proc.h>
#include <stdio.h>
#include <syscall.h>
#include <x86.h>
#include <file.h>
#include <gcc.h>
#include <string.h>

#define exit(...) return __VA_ARGS__

// Simple file system tests

char linebuf[1024];
char args[8][1024];
int nargs;

void touch() {
    int fd;
    if ((fd = open(args[1], O_RDONLY)) >= 0) {
        close(fd);
        return;
    }
    fd = open(args[1], O_CREATE);
    if (fd >= 0) {
        close(fd);
        return;
    }
    printf("touch: cannot create file %s.\n", args);
}

void ls() {
    int retval = sys_ls(args[1]);
}

int process_command(void) {
    int len = getline(linebuf, 1024);
    int i = 0, j = 0;
    int curr_arg = 0;
    while (linebuf[i] != '\0') {
        if (linebuf[i] == ' ') {
            args[curr_arg][j] = '\0';
            curr_arg++;
            if (curr_arg >= 8) {
                printf("ckosh: this alpha version only supports commands with 7 arguments :(");
                return 0;
            }
            j = 0;
        } else {
            args[curr_arg][j] = linebuf[i];
            j++;
        }
        i++;
    }
    args[curr_arg][j] = '\0';
    nargs = curr_arg + 1;
    return 1;
}

void mkdir_() {
    int fd;

    // Check num args
    if (nargs != 2) {
        printf("mkdir: wrong number of argument\n");
        return;
    }

    // If mkdir is unsuccessful, return
    if ((fd = mkdir(args[1]) < 0)) {
        printf("mkdir: could not make directtory\n");
        return;
    }

    // Otherwise, it works and nothing else needs to be done
}

void cd() {
    int fd;

    // Check num args
    if (nargs != 2) {
        printf("chdir: wrong number of argument\n");
        return;
    }

    // If chdir is unsuccessful, return
    if ((fd = chdir(args[1]) < 0)) {
        printf("chdir: could not make directtory\n");
        return;
    }

    // Otherwise, it works and nothing else needs to be done
}

void cat() {
    int fd, i;
    char buf[10000 + 1];

    // Check num args
    if (nargs != 2) {
        printf("cat: wrong number of argument\n");
        return;
    }

    // If arg is a directory, won't work for opening for write
    if (sys_isdir(args[1]) == 1) {
        printf("cat: %s is a directory\n", args);
        return;
    }

    fd = open(args[1], O_RDONLY);

    // Exit if file open doesn't work
    if (fd < 0) {
        printf("cat: open file failed %s\n", args);
        close(fd);
        return;
    }

    i = read(fd, buf, 10000);

    // Exit if file is longer than 10000 bytes or read fails and returns -1
    // TODO: Should still return first 10000 bytes if file is larger than that size?
    if (i < 0) {
        printf("cat: read file failed %s\n", args);
        close(fd);
        return;
    }
    if (i > 10000) {
        printf("cat: file too big %s\n", args);
        close(fd);
        return;
    }
    buf[i] = '\0'; 

    //If everything passes, print out the contnent of the file
    printf("%s\n", buf);
    close(fd);
}

void pwd(void) {
    sys_pwd();
}

int main(int argc, char *argv[])
{
    printf("Welcome to ckosh!\n");
    printf("Type 'help' for available commands.\n");
    int fd = open("file.txt", O_CREATE | O_RDWR);
    write(fd, "hello world!\ngoro sucks large pp", 33);
    close(fd);
    while(1) {
        printf("$ ");
        if(!process_command()) {
            continue;
        }
        // printf("%d args\n", nargs);
        // for (int i = 0; i < 8; i++) {
            // printf("%s\n", args[i]);
        // }
        if (strcmp(args[0], "touch") == 0) {
            touch();
        }
        else if (strcmp(args[0], "ls") == 0) {
            ls();
        }
        else if (strcmp(args[0], "cat") == 0) {
            cat();
        }
        else if (strcmp(args[0], "mkdir") == 0) {
            mkdir_();
        }
        else if (strcmp(args[0], "cd") == 0) {
            cd();
        }
        else if (strcmp(args[0], "pwd") == 0) {
            pwd();
        }
    }
}
