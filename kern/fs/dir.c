#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/string.h>
#include "inode.h"
#include "dir.h"

// Directories

#define DIRENT_SIZE sizeof(struct dirent)

int dir_namecmp(const char *s, const char *t)
{
    return strncmp(s, t, DIRSIZ);
}

/**
 * Look for a directory entry in a directory.
 * If found, set *poff to byte offset of entry.
 * Caller must acquire lock
 */
struct inode *dir_lookup(struct inode *dp, char *name, uint32_t * poff)
{
    uint32_t off;
    struct dirent de;

    if (dp->type != T_DIR)
        KERN_PANIC("dir_lookup not DIR");
    // inode_lock(dp);
    //TODO
    for (off = 0; off < dp->size; off += DIRENT_SIZE) {
        inode_read(dp, (char *) &de, off, DIRENT_SIZE);
        if (de.inum != 0 && dir_namecmp(de.name, name) == 0) {
            if (poff != 0) {
                *poff = off;
            }
            // inode_unlock(dp);
            return inode_get(dp->dev, de.inum);
        }
    }
    // inode_unlock(dp);

    return 0;
}

/**
 * Write a new directory entry (name, inum) into the directory dp.
 * Caller must acquire lock
 */
int dir_link(struct inode *dp, char *name, uint32_t inum)
{
    // TODO: Check that name is not present.
    uint32_t result;
    struct inode* file_inode;
    uint32_t off;
    struct dirent de;
    
    file_inode = dir_lookup(dp, name, &result);
    if (file_inode) {
        //file found in directory already
        inode_put(file_inode);
        return -1;
    }



    // TODO: Look for an empty dirent.
    // inode_lock(dp);
    for (off = 0; off < dp->size; off += DIRENT_SIZE) {
        inode_read(dp, (char *) &de, off, DIRENT_SIZE);
        if (de.inum == 0) {
            de.inum = inum;
            strncpy(de.name, name, DIRSIZ);
            inode_write(dp, (char *) &de, off, DIRENT_SIZE);
            // inode_unlock(dp);
            return 0;
        }
    }

    KERN_PANIC("No unallocated subdirectory entry\n");
    return -1;
}
