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

struct inode *create(char *path, short type, short major, short minor);

void append(char *dest, char *thing) {
    int len = strlen(dest);
    int i;
    for (i = 0; thing[i] != '\0'; i++) {
        dest[i + len] = thing[i];
    }
    dest[i + len] = '\0';
}

struct inode* copy_file(struct inode* ip) {
    struct inode *copy_ip = inode_alloc(ip->dev, ip->type); //TODO error
    copy_ip->major = 0;
    copy_ip->minor = 0;
    copy_ip->nlink = 1;
    inode_update(copy_ip);
    char buf[ip->size];
    inode_lock(copy_ip);
    if (inode_read(ip, buf, 0, ip->size) != ip->size) {
        return 0;
    }
    inode_write(copy_ip, buf, 0, ip->size);
    inode_unlock(copy_ip);
    return copy_ip;
}

struct inode* copy_dir(struct inode* ip, struct inode* parent_dp) {
    struct inode *copy_ip = inode_alloc(ip->dev, ip->type);
    copy_ip->major = 0;
    copy_ip->minor = 0;
    copy_ip->nlink = 1;
    inode_update(copy_ip);

    parent_dp->nlink++;      // for ".."
    inode_update(parent_dp);
    // Create . and .. entries.
    inode_lock(copy_ip);
                // No ip->nlink++ for ".": avoid cyclic ref count.
    if (dir_link(copy_ip, ".", copy_ip->inum) < 0
        || dir_link(copy_ip, "..", parent_dp->inum) < 0)
        KERN_PANIC("create dots");
    
    inode_unlock(copy_ip);
    return copy_ip;
}

int cp_recursive_helper(struct inode *src_dp, struct inode *dest_dp) {
    uint32_t off;
    struct dirent de;
    struct inode *ip, *copy_ip;

    inode_lock(src_dp);
    inode_lock(dest_dp);

    for (off = 0; off < src_dp->size; off += DIRENT_SIZE) {
        inode_read(src_dp, (char *) &de, off, DIRENT_SIZE);
        if (de.inum != 0 && strncmp(de.name, "..", DIRSIZ) && strncmp(de.name, ".", DIRSIZ)) {
            //found a directory entry
            ip = inode_get(src_dp->dev, de.inum);
            inode_lock(ip);

            if (ip->type == T_FILE) {
                begin_trans();
                copy_ip = copy_file(ip);
                dir_link(dest_dp, de.name, copy_ip->inum);
                commit_trans();
                inode_unlockput(ip);
            } 

            else if (ip->type == T_DIR) {
                //create new dest dir
                begin_trans();
                copy_ip = copy_dir(ip, dest_dp);
                dir_link(dest_dp, de.name, copy_ip->inum);
                commit_trans();
                inode_unlockput(ip);
                cp_recursive_helper(ip, copy_ip);
            }
            //ip is unlocked in if/else
        }
    }

    inode_unlockput(src_dp);
    inode_unlockput(dest_dp);
}

// extern struct inode *create(char *path, short type, short major, short minor);

void sys_cp_recursive(tf_t *tf) {
    char name[DIRSIZ], old[128], new[128];
    struct inode *dest_dp, *src_dp;
    int old_len, new_len;

    old_len = syscall_get_arg4(tf);
    new_len = syscall_get_arg5(tf);
    pt_copyin(get_curid(), syscall_get_arg2(tf), old, old_len);
    old[old_len] = '\0';
    pt_copyin(get_curid(), syscall_get_arg3(tf), new, new_len);
    new[new_len] = '\0';

    //create new if it doesn't exist
    if ((dest_dp = namei(new)) == 0) {
        syscall_set_errno(tf, E_BADF);
        return;
    }
    begin_trans();
    // if ((dest_dp = (struct inode *) create(new, T_DIR, 0, 0)) == 0) {
    //     commit_trans();
    //     syscall_set_errno(tf, E_DISK_OP);
    //     return;
    // }
    if (dest_dp->type != T_DIR) {
        //dest not directory
        syscall_set_errno(tf, E_BADF);
        return;
    }
    if ((src_dp = namei(old)) == 0 || src_dp->type != T_DIR) {
        //src not directory
        syscall_set_errno(tf, E_BADF);
        return;
    }
    cp_recursive_helper(src_dp, dest_dp);
}