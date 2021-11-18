#include <string.h>
#include <stdio.h>
#define DIRSIZ 14


static char *skipelem(char *path, char *name)
{
    int i, namei; 
    char c;
    i = 0; namei = 0;
    
    // Remove leading slashes
    while (path[i] == '/') {
        i++;
    }

    // Failure
    if (path[i] == '\0') {
        return (char *) 0;
    }

    for (namei = 0; (path[i + namei] != '\0' && path[i+namei] != '/'); namei++) {
        if (namei < DIRSIZ - 1) {
            name[namei] = path[i + namei];
        }
        if (namei == DIRSIZ - 1) {
            name[namei] = '\0';
        }
    }

    if (namei < DIRSIZ - 1) {
        name[namei] = '\0';
    }

    i += namei;

    // Remove leading slashes
    while (path[i] == '/') {
        i++;
    }  

    return path + i;
}

int main() {
    char * path = "//12345678901234567//b/c/d/";
    char name[DIRSIZ];
    path = skipelem(path, name);
    if (path == 0) {
        printf("XD\n");
    }
    else {
        printf("%s %s\n", path, name);
    }
    // char * path = "////";
    // char name[DIRSIZ];
    // path = skipelem(path, name);
    // if (path == 0) {
    //     printf("XD\n");
    // }
    // else {
    //     printf("%s %s\n", path, name);
    // }
    // char * path = "//aa///bb/c";
    // char name[DIRSIZ];
    // path = skipelem(path, name);
    // if (path == 0) {
    //     printf("XD\n");
    // }
    // else {
    //     printf("%s %s\n", path, name);
    // }
    // char * path = "abcde";
    // char name[DIRSIZ];
    // path = skipelem(path, name);
    // if (path == 0) {
    //     printf("XD\n");
    // }
    // else {
    //     printf("%s %s\n", path, name);
    // }
    // char * path = "/abcde";
    // char name[DIRSIZ];
    // path = skipelem(path, name);
    // if (path == 0) {
    //     printf("XD\n");
    // }
    // else {
    //     printf("%s %s\n", path, name);
    // }
    // char * path = "//12345678901234567//b/c/d/";
    // char name[DIRSIZ];
    // path = skipelem(path, name);
    // if (path == 0) {
    //     printf("XD\n");
    // }
    // else {
    //     printf("%s %s\n", path, name);
    // }
}