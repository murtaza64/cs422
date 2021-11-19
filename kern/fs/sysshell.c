// File-system system calls.

#include <kern/lib/types.h>
#include <kern/lib/debug.h>
#include <kern/lib/pmap.h>
#include <kern/lib/string.h>
#include <kern/lib/trap.h>
#include <kern/lib/syscall.h>
#include <kern/fs/params.h>
#include <kern/lib/spinlock.h>
#include <kern/thread/PTCBIntro/export.h>
#include <kern/thread/PCurID/export.h>
#include <kern/trap/TSyscallArg/export.h>

#include "dir.h"
#include "path.h"
#include "file.h"
#include "fcntl.h"
#include "log.h"
#include "sysshell.h"

#define FILEBUF_SIZE 10000
#define DIRENT_SIZE sizeof(struct dirent)

void sysshell_init() {
}

// Should fail if file doesn't exist and should also fail if its a file
void sys_isdir(tf_t *tf) {
    
    struct inode *ip;
    char path[128];
    int path_len;

    path_len = syscall_get_arg3(tf);
    pt_copyin(get_curid(), syscall_get_arg2(tf), path, path_len);
    path[path_len] = '\0';
    
    if ((ip = namei(path)) == 0) {
        syscall_set_retval1(tf, -2);
        syscall_set_errno(tf, E_NEXIST);
        return;
    }
    if (ip->type == T_DIR) {
        syscall_set_retval1(tf, 1);
        syscall_set_errno(tf, E_SUCC);
        return;
    }
    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
    return;
}

void sys_ls(tf_t *tf) {
    // KERN_INFO("IN LS!\n");
    //TODO: support arguments
    struct inode *dp = inode_dup((struct inode *) tcb_get_cwd(get_curid()));

    uint32_t off;
    struct dirent de;
    struct inode* ip;

    inode_lock(dp);
    // KERN_INFO("lock acquired\n");
    for (off = 0; off < dp->size; off += DIRENT_SIZE) {
        // KERN_INFO("loop start\n");
        inode_read(dp, (char *) &de, off, DIRENT_SIZE);
        if (de.inum != 0) {
            ip = inode_get(dp->dev, de.inum);
            //exception for . and ..
            if (ip->inum != dp->inum) {
                inode_lock(ip);
            }
            if (ip->type == T_FILE) {
                KERN_INFO("%s\n", de.name);
            }
            if (ip->type == T_DIR) {
                KERN_INFO("%s/\n", de.name);
            }
            if (ip->inum != dp->inum) {
                inode_unlockput(ip);
            }
        }
    }
    // KERN_INFO("END OF LS!\n");
    inode_unlockput(dp);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
}

void sys_pwd(tf_t *tf) {
    struct inode *current_dp = inode_dup((struct inode *) tcb_get_cwd(get_curid()));

    char path[128];
    path[127] = '\0';
    int pi = 126;

    struct inode* parent_dp;
    uint32_t off;
    struct dirent de;
    int len;
    int flag = 0;
    inode_lock(current_dp);
    while (current_dp->inum != ROOTINO) { //root dir
        parent_dp = dir_lookup(current_dp, "..", 0);
        inode_lock(parent_dp);
        for (off = 0; off < parent_dp->size; off += DIRENT_SIZE) {
            inode_read(parent_dp, (char *) &de, off, DIRENT_SIZE);
            if (de.inum == current_dp->inum) {
                len = strlen(de.name);
                for (int i = len - 1; i >= 0; i--, pi--) {
                    path[pi] = de.name[i];
                }
                path[pi--] = '/';
                if (pi < 0) {
                    KERN_PANIC("constructed path too long");
                }
                flag = 1;
                break;
            }
        }
        if (!flag) {
            KERN_PANIC("directory not found in parent");
        }
        inode_unlockput(current_dp);
        current_dp = parent_dp;
    }
    inode_unlock(current_dp);
    KERN_INFO("%s\n", path + pi + 1);
    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
}