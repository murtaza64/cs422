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
    char *path;
    if (nargs == 1) {
        path = ".";
    }
    else if (nargs == 2) {
        path = args[1];
    }
    else {
        printf("ls: accepts zero or one argument(s).\n");
        return;
    }
    if(sys_ls(path) < 0) {
        printf("ls: no such directory.\n");
    }
}

int process_command(void) {
    getline(linebuf, 1024);
    int i = 0, j = 0;
    int curr_arg = 0;
    int quote_flag = 0;
    int space_flag = 1;
    while (linebuf[i] != '\0') {
        if (linebuf[i] == '"') {
            if (quote_flag == 0) {
                // seen for first time, turn off space flag
                space_flag = 0;
                quote_flag = 1;
            } else {
                // seen for second time, turn off space flag
                space_flag = 1;
                quote_flag = 0;
            }
        } else if (linebuf[i] == ' ' && space_flag) {
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

    // If arg is a directory, return
    if (sys_isdir(args[1]) == 1) {
        printf("cat: %s is a directory\n", args[1]);
        return;
    }

    fd = open(args[1], O_RDONLY);

    // Exit if file open doesn't work
    if (fd < 0) {
        printf("cat: open file failed %s\n", args[1]);
        close(fd);
        return;
    }

    i = read(fd, buf, 10000);

    // Exit if file is longer than 10000 bytes or read fails and returns -1
    // TODO: Should still return first 10000 bytes if file is larger than that size?
    if (i < 0) {
        printf("cat: read file failed %s\n", args[1]);
        close(fd);
        return;
    }
    if (i > 10000) {
        printf("cat: file too big %s\n", args[1]);
        close(fd);
        return;
    }
    buf[i] = '\0'; 

    //If everything passes, print out the contnent of the file
    printf("%s\n", buf);
    close(fd);
}

char* lastname(char *path) {
    int len = strlen(path);
    while (path[len] != '/' && len > 0) {
        len--;
    }
    if (path[len] == '/') {
        len++;
    }
    return path + len;
}

void append(char *dest, char *thing) {
    int len = strlen(dest);
    int i;
    for (i = 0; thing[i] != '\0'; i++) {
        dest[i + len] = thing[i];
    }
    dest[i + len] = '\0';
}

int mv_movefile(char* old, char* new) {
    if (link(old, new) < 0) {
        printf("mv: no such file or directory\n");
        return 0;
    }
    if (unlink(old) < 0) {
        printf("mv: fatal: unable to unlink old file\n");
        return 0;
    }
    return 1;
}

// Example: $ writestr "insert string" file.txt
void writestr(void) {
    int fd, new_fd;
    char* insert_string = args[1];
    char* file_path = args[2];

    // Check num args
    if (nargs != 3) {
        printf("writestr: wrong number of argument\n");
        return;
    }

    // If filepath is a directory, return
    if (sys_isdir(file_path) == 1) {
        printf("writestr: %s is a directory\n", file_path);
        return;
    }

    // If file exists, then delete and create new one to empty out contents
    if ((fd = open(file_path, O_RDONLY)) >= 0) {
        close(fd);
        if (unlink(file_path) < 0) {
            printf("writestr: fatal: unable to unlink old file\n");
            return;
        }
    }

    // Then, create fresh new file with nothing inside
    if ((new_fd = open(file_path, O_CREATE | O_RDWR) < 0)) {
        printf("writestr: fatal: unable to create new file\n");
        // close(fd);
        close(new_fd);
        return;
    }

    // Write the string to the new file
    if (write(new_fd, insert_string, strlen(insert_string)) < 0) {
        printf("writestr: fatal: unable to write to new file\n");
        // close(fd);
        close(new_fd);
        return;
    }

    close(new_fd);
    // close(fd);
}

// Example: $ appendstr "insert string" file.txt
void appendstr(void) {
    int fd, new_fd, i, j;
    char* insert_string = args[1];
    char* file_path = args[2];
    int string_len = strlen(insert_string);
    char buf[10000 + 1];

    // Check num args
    if (nargs != 3) {
        printf("appendstr: wrong number of argument\n");
        return;
    }

    // If filepath is a directory, return
    if (sys_isdir(file_path) == 1) {
        printf("appendstr: %s is a directory\n", file_path);
        return;
    }

    // If file does not exist, then fails
    if ((fd = open(file_path, O_RDWR)) < 0) {
        printf("appendstr: file %s does not exist\n", file_path);
        close(fd);
        return;
    }

    // Exit if file open doesn't work
    if (fd < 0) {
        printf("appendstr: open file failed %s\n", args);
        close(fd);
        return;
    }

    // Read from file into buffer
    i = read(fd, buf, 10000);

    // Exit if file is longer than 10000 bytes or read fails and returns -1
    // TODO: Should still return first 10000 bytes if file is larger than that size?
    if (i < 0) {
        printf("appendstr: read file failed %s\n", args);
        close(fd);
        return;
    }
    if (i + string_len > 10000) {
        printf("appendstr: file too big %s\n", args);
        close(fd);
        return;
    }

    // Append the string to the buffer
    for (j = 0; j < string_len; j++) {
        buf[i] = insert_string[j];
        i++;
    }

    // Delete and create new file to empty out contents
    close(fd);
    if (unlink(file_path) < 0) {
        printf("appendstr: fatal: unable to unlink old file\n");
        return;
    }

    // Then, create fresh new file with nothing inside
    if ((new_fd = open(file_path, O_CREATE | O_RDWR) < 0)) {
        printf("appendstr: fatal: unable to create new file\n");
        close(new_fd);
        return;
    }

    // Write the string to the new file
    if (write(new_fd, buf, i) < 0) {
        printf("appendstr: fatal: unable to write to new file\n");
        close(new_fd);
        return;
    }

    close(new_fd);
}

void rm(void) {
    if (strcmp(args[1], "-r") == 0 && nargs == 3) {
        if (!sys_isdir(args[2])) {
            printf("rm: no such directory\n");
            return;
        }
        if (sys_rm_recursive(args[2]) < 0) {
            printf("rm: something went wrong\n");
        }
        return;
    }

    // Check num args for non-recursive case
    if (nargs != 2) {
        printf("rm: wrong number of arguments\n");
        return;
    }

    if (sys_isdir(args[1])) {
        printf("rm: use -r to remove directory\n");
        return;
    }

    // If unlinking file doesn't work, return
    if (unlink(args[1]) < 0) {
        printf("rm: fatal: unable to unlink old file\n");
        return;
    }
    // Otherwise, it works and nothing else needs to be done
}

void mv(void) {
    // link dest to source file
    // unlink source
    // if (strcmp(args[1], "-r") == 0 && nargs == 4) {
    //     mv_recursive(args[2], args[3]);
    //     return;
    // }

    if (nargs != 3) {
         printf("mv: wrong number of arguments\n");
         return;
    }

    char* old = args[1];
    char* new = args[2];

    if (sys_isdir(new)) {
        append(new, "/");
        append(new, lastname(old));
    }

    printf("mv: %s -> %s\n", old, new);

    if (sys_isdir(old)) {
        if(sys_mv_dir(old, new) < 0) {
            printf("mv: something went wrong");
        }
        return;
    }



    mv_movefile(old, new);
}

int cp_copyfile(char* old, char* new) {
    char buf[10000];
    int old_fd, new_fd;

    if ((old_fd = open(old, O_RDONLY)) < 0) {
        printf("cp: cannot open file %s\n", old);
        close(old_fd);
        return 0;
    }
    
    if ((new_fd = open(new, O_CREATE | O_RDWR)) < 0) {
        printf("cp: cannot create file %s\n", new);
        close(old_fd);
        close(new_fd);
        return 0;
    }
    int i = 0;
    do {
        i = read(old_fd, buf, 10000);
        if (i < 0) {
            printf("cp: fatal: unable to read file\n", "%s");
            close(old_fd);
            return 0;
        }
        write(new_fd, buf, i);
    } while (i > 0);
    close(new_fd);
    close(old_fd);
    return 1;
}

void cp(void) {
    // link dest to source file
    // unlink source
    
    if (strcmp(args[1], "-r") == 0 && nargs == 4) {
        char* old = args[2];
        char* new = args[3];
        int new_fd;
        if (!sys_isdir(old)) {
            printf("cp: source file %s is not a directory.\n", old);
        }
        if ((new_fd = open(new, O_RDONLY)) < 0) {
            mkdir(new);
        }
        else if (sys_isdir(new)) {
            //dest directory already exists, so create new 
            //directory with name of source directory
            close(new_fd);
            
            append(new, "/");
            append(new, lastname(old));
            if ((new_fd = open(new, O_RDONLY)) < 0) {
                mkdir(new);
            }
        }

        if (!sys_isdir(new)) {
            printf("cp: dest file %s is not a directory.\n", new);
            return;
        }
        if(sys_cp_recursive(old, new) < 0) {
            printf("cp: something went wrong");
        }
        return;
    }

    if (nargs != 3) {
         printf("cp: wrong number of arguments\n");
         return;
    }

    char* old = args[1];
    char* new = args[2];

    if (sys_isdir(new)) {
        append(new, "/");
        append(new, lastname(old));
    }

    if (sys_isdir(old)) {
        printf("cp: source file is a directory. use cp -r to copy recursively\n");
        return;
    }

    printf("cp: %s -> %s\n", old, new);
    cp_copyfile(old, new);
}

void pwd(void) {
    sys_pwd();
}

int main(int argc, char *argv[])
{
    // printf("initializing ckosh...\n");
    // mkdir("test");
    // int fd = open("test/file.txt", O_CREATE | O_RDWR);
    // write(fd, "hello world!\n", 32);
    // close(fd);

    // mkdir("deep");
    // mkdir("deep/deep2");
    // mkdir("deep/deep2/deep3");
    // mkdir("deep/deep2a");
    // fd = open("deep/file0.txt", O_CREATE);
    // close(fd);
    // fd = open("deep/deep2/file2.txt", O_CREATE);
    // close(fd);
    // fd = open("deep/deep2a/file2a.txt", O_CREATE);
    // close(fd);
    // fd = open("deep/deep2/deep3/file3.txt", O_CREATE);
    // close(fd);

    // mkdir("beep");
    // mkdir("beep/deep2a");
    // fd = open("beep/deep2a/file2a.txt", O_CREATE);
    // close(fd);
    // fd = open("beep/file1.txt", O_CREATE);
    // close(fd);

    printf("\n");
    printf("Welcome to ckosh!\n");

    while(1) {
        printf("$ ");
        if(!process_command()) {
            continue;
        }
        // for (int i = 0; i < nargs; i++) {
        //     printf("args[%d] = \"%s\"\n", i, args[i]);
        // }
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
        else if (strcmp(args[0], "mv") == 0) {
            mv();
        }
        else if (strcmp(args[0], "rm") == 0) {
            rm();
        }
        else if (strcmp(args[0], "writestr") == 0) {
            writestr();
        }
        else if (strcmp(args[0], "appendstr") == 0) {
            appendstr();
        }
        else if (strcmp(args[0], "cp") == 0) {
            cp();
        }
        else {
            printf("command %s not supported\n", args[0]);
        }
    }
}
