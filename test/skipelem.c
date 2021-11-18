#include <string.h>
#include <stdio.h>
#define DIRSIZ 14

static char *skipelem(char *path, char *name)
{
    // TODO
    int i, k;
    i = 0;
    //remove leading slashes
    for (; path[i] == '/'; i++);
    
    // Save name after first set of slashes
    k = 0;
    while (path[i] != '/' && path[i] != '\0') {
        //only copy the first DIRSIZ - 1, but keep consuming till a / or \0
        if (k < DIRSIZ - 1) {
            name[k] = path[i];
            k++;
        }
        i++;
    }
    // Add null terminator to end of name
    name[k] = '\0';

    // No non-slash characters
    if (k == 0) {
        return 0;
    }
    
    
    //remove next set of leading slashes
    for (; path[i] == '/'; i++);

    // Copy over remaining path after second set of slashes
    // char retpath[strlen(path) + 1]; // [strlen(path) - i + 1]
    // j = 0;
    // while (i <= strlen(path)) {
    //     retpath[j] = path[i];
    //     i++;
    //     j++;
    // }

    return path + i;
}


int main() {
    char * path = "/home/user/daniel/taxes";
    char name[DIRSIZ];
    while ((path = skipelem(path, name)) != 0) {
        // TODO
        printf("path=%s, name=%s\n", path, name);
    }
}