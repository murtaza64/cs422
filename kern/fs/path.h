#ifndef _KERN_FS_PATH_H_
#define _KERN_FS_PATH_H_

#ifdef _KERN_

#define DIRSIZ 14

struct inode *namei(char *path);
struct inode *nameiparent(char *path, char *name);
unsigned int strlen(const char* s);

#endif  /* _KERN_ */

#endif  /* !_KERN_FS_PATH_H_ */
