#ifndef _GENSHINFS_H_
#define _GENSHINFS_H_

#define FUSE_USE_VERSION 26
#include "stdio.h"
#include "stdlib.h"
#include <unistd.h>
#include "fcntl.h"
#include "string.h"
#include "fuse.h"
#include <stddef.h>
#include "ddriver.h"
#include "errno.h"
#include "types.h"

#define GENSHINFS_MAGIC                  /* TODO: Define by yourself */
#define GENSHINFS_DEFAULT_PERM    0777   /* 全权限打开 */

/******************************************************************************
* SECTION: genshinfs.c
*******************************************************************************/
void* 			   genshinfs_init(struct fuse_conn_info *);
void  			   genshinfs_destroy(void *);
int   			   genshinfs_mkdir(const char *, mode_t);
int   			   genshinfs_getattr(const char *, struct stat *);
int   			   genshinfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   genshinfs_mknod(const char *, mode_t, dev_t);
int   			   genshinfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   genshinfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   genshinfs_access(const char *, int);
int   			   genshinfs_unlink(const char *);
int   			   genshinfs_rmdir(const char *);
int   			   genshinfs_rename(const char *, const char *);
int   			   genshinfs_utimens(const char *, const struct timespec tv[2]);
int   			   genshinfs_truncate(const char *, off_t);
			
int   			   genshinfs_open(const char *, struct fuse_file_info *);
int   			   genshinfs_opendir(const char *, struct fuse_file_info *);

#endif  /* _genshinfs_H_ */