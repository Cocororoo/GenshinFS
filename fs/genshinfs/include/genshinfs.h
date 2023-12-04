#ifndef _GFS_H_
#define _GFS_H_

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



/******************************************************************************
* SECTION: genshinfs.c
*******************************************************************************/
void* 			   gfs_init(struct fuse_conn_info *);
void  			   gfs_destroy(void *);
int   			   gfs_mkdir(const char *, mode_t);
int   			   gfs_getattr(const char *, struct stat *);
int   			   gfs_readdir(const char *, void *, fuse_fill_dir_t, off_t,
						                struct fuse_file_info *);
int   			   gfs_mknod(const char *, mode_t, dev_t);
int   			   gfs_write(const char *, const char *, size_t, off_t,
					                  struct fuse_file_info *);
int   			   gfs_read(const char *, char *, size_t, off_t,
					                 struct fuse_file_info *);
int   			   gfs_access(const char *, int);
int   			   gfs_unlink(const char *);
int   			   gfs_rmdir(const char *);
int   			   gfs_rename(const char *, const char *);
int   			   gfs_utimens(const char *, const struct timespec tv[2]);
int   			   gfs_truncate(const char *, off_t);
			
int   			   gfs_open(const char *, struct fuse_file_info *);
int   			   gfs_opendir(const char *, struct fuse_file_info *);

/******************************************************************************
* SECTION: genshinfs_utils.c
*******************************************************************************/
char* 			   gfs_get_fname(const char* path);
int 			   gfs_calc_lvl(const char * path);
int 			   gfs_driver_read(int offset, uint8_t *out_content, int size);
int 			   gfs_driver_write(int offset, uint8_t *in_content, int size);


int 			   gfs_mount(struct custom_options options);
int 			   gfs_umount();

int 			   gfs_alloc_dentry(struct gfs_inode * inode, struct gfs_dentry * dentry);
int 			   gfs_drop_dentry(struct gfs_inode * inode, struct gfs_dentry * dentry);
struct gfs_inode*  gfs_alloc_inode(struct gfs_dentry * dentry);
int 			   gfs_sync_inode(struct gfs_inode * inode);
int 			   gfs_drop_inode(struct gfs_inode * inode);
struct gfs_inode*  gfs_read_inode(struct gfs_dentry * dentry, int ino);
struct gfs_dentry* gfs_get_dentry(struct gfs_inode * inode, int dir);

struct gfs_dentry* gfs_lookup(const char * path, boolean * is_find, boolean* is_root);


/******************************************************************************
* SECTION: debug
*******************************************************************************/
#define GFS_DBG(fmt, ...) do { printf("GFS_DBG: " fmt, ##__VA_ARGS__); } while(0) 
void 			   gfs_dump_inode_map();
void 			   gfs_dump_data_map();
void 			   gfs_dump_inode(struct gfs_inode * inode);
void 			   gfs_dump_dentry(struct gfs_dentry * dentry);
void 			   gfs_dump_super();
#endif  /* _gfs_H_ */