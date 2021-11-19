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
        inode_put(ip);
        syscall_set_retval1(tf, -2);
        syscall_set_errno(tf, E_NEXIST);
        return;
    }
    inode_lock(ip);
    if (ip->type == T_DIR) {
        inode_unlockput(ip);
        syscall_set_retval1(tf, 1);
        syscall_set_errno(tf, E_SUCC);
        return;
    }
    inode_unlockput(ip);
    syscall_set_retval1(tf, 0);
    syscall_set_errno(tf, E_SUCC);
    return;
}

void sys_ls(tf_t *tf) {
    // KERN_INFO("IN LS!\n");
    //TODO: support arguments
    char path[128];
    int path_len;
    path_len = syscall_get_arg3(tf);
    pt_copyin(get_curid(), syscall_get_arg2(tf), path, path_len);
    path[path_len] = '\0';

    struct inode *dp = namei(path);
    if (dp == 0) {
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
        return;
    }

    inode_lock(dp);
    if (dp->type != T_DIR) {
        // KERN_WARN("ls: dp = %d, dp type = %d\n", dp, dp->type);
        inode_unlockput(dp);
        syscall_set_errno(tf, E_BADF);
        syscall_set_retval1(tf, -1);
        return;
    }

    uint32_t off;
    struct dirent de;
    struct inode* ip;

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
    // KERN_INFO("in copy_file\n");
    struct inode *copy_ip = inode_alloc(ip->dev, ip->type); //TODO error
    if (copy_ip == 0) {
        KERN_PANIC("alloc inode");
    }
    inode_lock(copy_ip);
    copy_ip->major = 0;
    copy_ip->minor = 0;
    copy_ip->nlink = 1;
    inode_update(copy_ip);
    char buf[ip->size];
    if (inode_read(ip, buf, 0, ip->size) != ip->size) {
        KERN_PANIC("copy read inode");
    }
    if (inode_write(copy_ip, buf, 0, ip->size) != ip->size) {
        KERN_PANIC("copy write inode");
    }
    // inode_unlock(copy_ip);
    KERN_INFO("copy_dir completed\n");
    return copy_ip;
}

struct inode* copy_dir(struct inode* ip, struct inode* parent_dp) {
    struct inode *copy_ip = inode_alloc(ip->dev, ip->type);
    if (copy_ip == 0) {
        KERN_PANIC("alloc inode");
    }
    KERN_INFO("new dir: inode=%d refs=%d\n", ip->inum, ip->ref);
    inode_lock(copy_ip);
    copy_ip->major = 0;
    copy_ip->minor = 0;
    copy_ip->nlink = 1;
    inode_update(copy_ip);

    parent_dp->nlink++;      // for ".."
    inode_update(parent_dp);
    // Create . and .. entries.
                // No ip->nlink++ for ".": avoid cyclic ref count.
    KERN_INFO("creating . and ..\n");
    if (dir_link(copy_ip, ".", copy_ip->inum) < 0
        || dir_link(copy_ip, "..", parent_dp->inum) < 0)
        KERN_PANIC("create dots");
    
    // inode_unlock(copy_ip);
    KERN_INFO("finished copy_dir\n");
    return copy_ip;
}

void cp_info(char *src_path, char *dest_path, char *name) {
    KERN_INFO("cp: %s/%s -> %s/%s\n", src_path, name, dest_path, name);
}
void cp_info_overwrite(char *src_path, char *dest_path, char *name) {
    KERN_INFO("cp: %s/%s -> %s/%s (overwrite)\n", src_path, name, dest_path, name);
}
void cp_info_dir(char *src_path, char *dest_path, char *name) {
    KERN_INFO("cp: %s/%s/ -> %s/%s/\n", src_path, name, dest_path, name);
}
void cp_info_dir_merge(char *src_path, char *dest_path, char *name) {
    KERN_INFO("cp: %s/%s/ -> %s/%s/ (merge)\n", src_path, name, dest_path, name);
}

int cp_recursive_helper(struct inode *src_dp, struct inode *dest_dp, char *src_path, char *dest_path) {
    uint32_t off;
    struct dirent de;
    struct inode *ip, *copy_ip, *existing_ip;
    // KERN_INFO("in cp recursive helper\n");
    inode_lock(src_dp);
    inode_lock(dest_dp);

    for (off = 0; off < src_dp->size; off += DIRENT_SIZE) {
        if (inode_read(src_dp, (char *) &de, off, DIRENT_SIZE) != DIRENT_SIZE) {
            KERN_PANIC("inode read");
        }
        if (de.inum != 0 && strncmp(de.name, "..", DIRSIZ) && strncmp(de.name, ".", DIRSIZ)) {
            //found a directory entry
            // KERN_INFO("copying file %s\n", de.name);
            ip = inode_get(src_dp->dev, de.inum);
            if (ip == 0) {
                KERN_PANIC("inode get");
            }
            inode_lock(ip);
            if (ip->type == T_FILE) {
                begin_trans();
                copy_ip = copy_file(ip);
                // inode_lock(copy_ip);
                if ((existing_ip = dir_lookup(dest_dp, de.name, 0)) != 0) {
                    cp_info_overwrite(src_path, dest_path, de.name);
                    inode_lock(existing_ip);
                    existing_ip->nlink--;
                    inode_update(existing_ip);
                    inode_unlockput(existing_ip);
                }
                else {
                    cp_info(src_path, dest_path, de.name);
                }
                dir_link(dest_dp, de.name, copy_ip->inum);
                inode_unlockput(ip);
                inode_unlockput(copy_ip);
                commit_trans();
            } 

            else if (ip->type == T_DIR) {
                //create new dest dir
                begin_trans();
                if((existing_ip = dir_lookup(dest_dp, de.name, 0)) != 0) {
                    //TODO: handle case where existing file with name is not a directory
                    cp_info_dir_merge(src_path, dest_path, de.name);
                    copy_ip = existing_ip;
                }
                else {
                    cp_info_dir(src_path, dest_path, de.name);
                    copy_ip = copy_dir(ip, dest_dp);
                    // inode_lock(copy_ip);
                    dir_link(dest_dp, de.name, copy_ip->inum);
                    copy_ip->nlink++;
                    inode_update(copy_ip);
                    inode_unlock(copy_ip);
                }
                inode_unlock(ip);
                commit_trans();

                char src_new_path[strlen(src_path) + strlen(de.name) + 2];
                strncpy(src_new_path, src_path, strlen(src_path) + 1);
                append(src_new_path, "/");
                append(src_new_path, de.name);

                char dest_new_path[strlen(dest_path) + strlen(de.name) + 2];
                strncpy(dest_new_path, dest_path, strlen(dest_path) + 1);
                append(dest_new_path, "/");
                append(dest_new_path, de.name);

                cp_recursive_helper(ip, copy_ip, src_new_path, dest_new_path);
                inode_put(ip);
                inode_put(copy_ip);
            }
            //ip is unlocked in if/else
        }
    }

    inode_unlock(src_dp);
    inode_unlock(dest_dp);
}

// extern struct inode *create(char *path, short type, short major, short minor);

void sys_cp_recursive(tf_t *tf) {
    char old[128], new[128];
    struct inode *dest_dp, *src_dp;
    int old_len, new_len;

    old_len = syscall_get_arg4(tf); //TODO: error checks for path lengths
    new_len = syscall_get_arg5(tf);
    pt_copyin(get_curid(), syscall_get_arg2(tf), old, old_len);
    old[old_len] = '\0';
    pt_copyin(get_curid(), syscall_get_arg3(tf), new, new_len);
    new[new_len] = '\0';

    //insist new already exists
    if ((dest_dp = namei(new)) == 0) {
        syscall_set_errno(tf, E_BADF);
        return;
    }
    inode_lock(dest_dp);
    if (dest_dp->type != T_DIR) {
        //dest not directory
        inode_unlockput(dest_dp);
        syscall_set_errno(tf, E_BADF);
        return;
    }
    ;
    if ((src_dp = namei(old)) == 0) {
        //src can't be opened
        syscall_set_errno(tf, E_BADF);
        return;
    }
    inode_lock(src_dp);
    if (src_dp->type != T_DIR) {
        inode_unlockput(src_dp);
        inode_unlockput(dest_dp);
        syscall_set_errno(tf, E_BADF);
        return;
    }
    
    inode_unlock(src_dp);
    inode_unlock(dest_dp);
    cp_recursive_helper(src_dp, dest_dp, old, new);
    inode_put(src_dp);
    inode_put(dest_dp);

    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
}

void rm_info(char *path, char *name) {
    KERN_INFO("rm: unlink %s/%s\n", path, name);
}
void rm_info_dir(char *path, char *name) {
    KERN_INFO("rm: delete %s/%s/\n", path, name);
}

int rm_recursive_helper(struct inode *dp, char *path) {
    //TODO: more error cases
    uint32_t off;
    struct dirent de, zerode;
    struct inode *ip;
    inode_lock(dp);

    memset((void*) &zerode, 0, sizeof(zerode));

    for (off = 0; off < dp->size; off += DIRENT_SIZE) {
        if (inode_read(dp, (char *) &de, off, DIRENT_SIZE) != DIRENT_SIZE) {
            KERN_PANIC("inode read");
        }
        if (de.inum != 0 && strncmp(de.name, "..", DIRSIZ) && strncmp(de.name, ".", DIRSIZ)) {
            //found a directory entry
            ip = inode_get(dp->dev, de.inum);
            if (ip == 0) {
                KERN_PANIC("inode get");
            }
            inode_lock(ip);
            // KERN_INFO("name %s type %d\n", de.name, ip->type);
            if (ip->type != T_FILE && ip->type != T_DIR) {
                inode_unlockput(ip);
                continue;
            }
            if (ip->type == T_DIR) {
                //recursively delete
                rm_info_dir(path, de.name);
                char new_path[strlen(path) + strlen(de.name) + 2];
                strncpy(new_path, path, strlen(path) + 1);
                append(new_path, "/");
                append(new_path, de.name);
                inode_unlock(ip);
                rm_recursive_helper(ip, new_path);
                inode_lock(ip);
                begin_trans();
                //child dir's .. link
                dp->nlink--;
            }
            else {
                rm_info(path, de.name);
                begin_trans();
            }
            //update links
            ip->nlink--;
            inode_update(ip);
            inode_update(dp);

            //zero dirent
            inode_write(dp, (char *) &zerode, off, sizeof(zerode));
            
            inode_unlockput(ip);
            commit_trans();
            // KERN_INFO("end of transaction\n");
            //ip is unlocked in if/else
        }
    }

    inode_unlock(dp);
}

void sys_rm_recursive(tf_t *tf) {
    char path[128];
    char name[DIRSIZ];
    struct inode *dp, *parent_dp;
    int len;

    len = syscall_get_arg3(tf); //TODO: error checks for path lengths
    pt_copyin(get_curid(), syscall_get_arg2(tf), path, len);
    path[len] = '\0';

    if ((dp = namei(path)) == 0) {
        //src not directory
        syscall_set_errno(tf, E_BADF);
        return;
    }
    inode_lock(dp);
    if (dp->type != T_DIR) {
        inode_unlockput(dp);
        syscall_set_errno(tf, E_BADF);
        return;
    }
    if ((parent_dp = nameiparent(path, name)) == 0) {
        syscall_set_errno(tf, E_BADF);
        return;
    }

    inode_unlock(dp);
    rm_recursive_helper(dp, path);
    inode_put(dp);

    KERN_INFO("rm: unlink %s from parent\n", name);

    inode_lock(parent_dp);
    begin_trans();

    uint32_t off;
    struct dirent zerode;

    memset((void *) &zerode, 0, sizeof(zerode));
    dp = dir_lookup(parent_dp, name, &off);
    if (dp == 0) {
        KERN_PANIC("directory not found in parent");
    }
    inode_lock(dp);
    inode_write(parent_dp, (char *) &zerode, off, sizeof(zerode));
    // KERN_INFO("dp->nlink=%d, parent_dp->nlink=%d", dp->nlink, parent_dp->nlink);
    dp->nlink--;
    parent_dp->nlink--;
    // inode_update(dp);
    // inode_update(parent_dp);
    inode_unlockput(dp);
    inode_unlockput(parent_dp);
    commit_trans();

    syscall_set_errno(tf, E_SUCC);
    syscall_set_retval1(tf, 0);
}