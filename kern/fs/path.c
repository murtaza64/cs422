// File system implementation.  Five layers:
//   + Blocks: allocator for raw disk blocks.
//   + Log: crash recovery for multi-step updates.
//   + Files: inode allocator, reading, writing, metadata.
//   + Directories: inode with special contents (list of other inodes!)
//   + Names: paths like /usr/fs.c for convenient naming.
//
// This file contains the low-level file system manipulation
// routines.  The (higher-level) system call implementations
// are in sysfile.c.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/spinlock.h>
#include <thread/PTCBIntro/export.h>
#include <thread/PCurID/export.h>
#include "inode.h"
#include "dir.h"
#include "log.h"

#include <kern/lib/string.h>

// Paths

/**
 * Copy the next path element from path into name.
 * If the length of name is larger than or equal to DIRSIZ, then only
 * (DIRSIZ - 1) # characters should be copied into name.
 * This is because you need to save '\0' in the end.
 * You should still skip the entire string in this case.
 * Return a pointer to the element following the copied one.
 * The returned path has no leading slashes,
 * so the caller can check *path == '\0' to see if the name is the last one.
 * If no name to remove, return 0.
 *
 * Examples :
 *   skipelem("a/bb/c", name) = "bb/c", setting name = "a"
 *   skipelem("///a//bb", name) = "bb", setting name = "a"
 *   skipelem("a", name) = "", setting name = "a"
 *   skipelem("", name) = skipelem("////", name) = 0
 */

unsigned int strlen(const char* s) {
    int i;
    for (i = 0; s[i] != '\0'; i++);
    return i;
}

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

    //lol just do pointer arithmetic
    return path + i;
}

/**
 * Look up and return the inode for a path name.
 * If nameiparent is true, return the inode for the parent and copy the final
 * path element into name, which must have room for DIRSIZ bytes.
 * Returns 0 in the case of error.
 */
static struct inode *namex(char *path, bool nameiparent, char *name)
{
    struct inode *ip;
    struct inode *next;
    // struct inode *parent_ip;

    // If path is a full path, get the pointer to the root inode. Otherwise get
    // the inode corresponding to the current working directory.
    // parent_ip = 0;
    if (*path == '/') {
        ip = inode_get(ROOTDEV, ROOTINO);
    } else {
        ip = inode_dup((struct inode *) tcb_get_cwd(get_curid()));
    }
    //TODO: if path is empty, what should nameiparent do?
    //currently just returns current dir

    while ((path = skipelem(path, name)) != 0) {
        
        inode_lock(ip);
        if (ip->type != T_DIR) {
            return 0;
        }
        // inode_unlock(ip);
        if (nameiparent && path[0] == '\0') {
            inode_unlock(ip);
            break;
        }
        next = dir_lookup(ip, name, 0);
        inode_unlockput(ip);
        ip = next;
    }
    // if (nameiparent) {
    //     inode_put(ip);
    //     return ip;
    // }
    // inode_put(ip);
    return ip;
}

/**
 * Return the inode corresponding to path.
 */
struct inode *namei(char *path)
{
    char name[DIRSIZ];
    return namex(path, FALSE, name);
}

/**
 * Return the inode corresponding to path's parent directory and copy the final
 * element into name.
 */
struct inode *nameiparent(char *path, char *name)
{
    return namex(path, TRUE, name);
}
