#define _XOPEN_SOURCE 700
#include "genshinfs.h"

#define OPTION(t, p)        { t, offsetof(struct custom_options, p), 1 }

/******************************************************************************
* SECTION: 全局变量
*******************************************************************************/
static const struct fuse_opt option_spec[] = {		/* 用于FUSE文件系统解析参数 */
	OPTION("--device=%s", device),
	FUSE_OPT_END
};

struct custom_options gfs_options;			 /* 全局选项 */
struct gfs_super gfs_super; 
/******************************************************************************
* SECTION: FUSE操作定义
*******************************************************************************/
static struct fuse_operations operations = {
	.init = gfs_init,						 /* mount文件系统 */		
	.destroy = gfs_destroy,				 /* umount文件系统 */
	.mkdir = gfs_mkdir,					 /* 建目录，mkdir */
	.getattr = gfs_getattr,				 /* 获取文件属性，类似stat，必须完成 */
	.readdir = gfs_readdir,				 /* 填充dentrys */
	.mknod = gfs_mknod,					 /* 创建文件，touch相关 */
	.write = NULL,								  	 /* 写入文件 */
	.read = NULL,								  	 /* 读文件 */
	.utimens = gfs_utimens,				 /* 修改时间，忽略，避免touch报错 */
	.truncate = NULL,						  		 /* 改变文件大小 */
	.unlink = NULL,							  		 /* 删除文件 */
	.rmdir	= NULL,							  		 /* 删除目录， rm -r */
	.rename = NULL,							  		 /* 重命名，mv */

	.open = NULL,							
	.opendir = NULL,
	.access = NULL
};
/******************************************************************************
* SECTION: 必做函数实现
*******************************************************************************/
/**
 * @brief 挂载（mount）文件系统
 * 
 * @param conn_info 可忽略，一些建立连接相关的信息 
 * @return void*
 */
void* gfs_init(struct fuse_conn_info * conn_info) {
	/* TODO: 在这里进行挂载 */
	if (gfs_mount(gfs_options) != GFS_ERROR_NONE) {
        GFS_DBG("[%s] mount error\n", __func__);
		fuse_exit(fuse_get_context()->fuse);
		return NULL;
	} 
	return NULL;
}

/**
 * @brief 卸载（umount）文件系统
 * 
 * @param p 可忽略
 * @return void
 */
void gfs_destroy(void* p) {
	/* TODO: 在这里进行卸载 */
	
	if (gfs_umount() != GFS_ERROR_NONE) {
		GFS_DBG("[%s] unmount error\n", __func__);
		fuse_exit(fuse_get_context()->fuse);
		return;
	}
	return;
}

/**
 * @brief 创建目录
 * 
 * @param path 相对于挂载点的路径
 * @param mode 创建模式（只读？只写？），可忽略
 * @return int 0成功，否则失败
 */
int gfs_mkdir(const char* path, mode_t mode) {
	/* 解析路径，创建目录 */
	(void)mode;
	boolean is_find, is_root;
	char* fname;
	struct gfs_dentry* last_dentry = gfs_lookup(path, &is_find, &is_root);
	struct gfs_dentry* dentry;
	struct gfs_inode*  inode;

	if (is_find) {
		return -GFS_ERROR_EXISTS;
	}

	if (GFS_IS_REG(last_dentry->inode)) {
		return -GFS_ERROR_UNSUPPORTED;
	}

	fname  = gfs_get_fname(path);
	dentry = new_dentry(fname, GFS_DIR); 
	dentry->parent = last_dentry;
	inode  = gfs_alloc_inode(dentry);
	gfs_alloc_dentry(last_dentry->inode, dentry);
	
	return GFS_ERROR_NONE;
}

/**
 * @brief 获取文件或目录的属性，该函数非常重要
 * 
 * @param path 相对于挂载点的路径
 * @param gfs_stat 返回状态
 * @return int 0成功，否则失败
 */
int gfs_getattr(const char* path, struct stat * gfs_stat) {
	/* 解析路径，获取Inode，填充gfs_stat */
	boolean	is_find, is_root;
	struct gfs_dentry* dentry = gfs_lookup(path, &is_find, &is_root);
	if (is_find == FALSE) {
		return -GFS_ERROR_NOTFOUND;
	}

	if (GFS_IS_DIR(dentry->inode)) {
		gfs_stat->st_mode = S_IFDIR | GFS_DEFAULT_PERM;
		gfs_stat->st_size = dentry->inode->dir_cnt * sizeof(struct gfs_dentry_d);
	}
	else if (GFS_IS_REG(dentry->inode)) {
		gfs_stat->st_mode = S_IFREG | GFS_DEFAULT_PERM;
		gfs_stat->st_size = dentry->inode->size;
	}


	gfs_stat->st_nlink = 1;
	gfs_stat->st_uid 	 = getuid();
	gfs_stat->st_gid 	 = getgid();
	gfs_stat->st_atime   = time(NULL);
	gfs_stat->st_mtime   = time(NULL);
	gfs_stat->st_blksize = GFS_BLK_SZ();

	if (is_root) {
		gfs_stat->st_size	= gfs_super.usage; 
		gfs_stat->st_blocks = GFS_DISK_SZ() / GFS_BLK_SZ();
		gfs_stat->st_nlink  = 2;		/* !特殊，根目录link数为2 */
	}
	return GFS_ERROR_NONE;
}

/**
 * @brief 遍历目录项，填充至buf，并交给FUSE输出
 * 
 * @param path 相对于挂载点的路径
 * @param buf 输出buffer
 * @param filler 参数讲解:
 * 
 * typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
 *				const struct stat *stbuf, off_t off)
 * buf: name会被复制到buf中
 * name: dentry名字
 * stbuf: 文件状态，可忽略
 * off: 下一次offset从哪里开始，这里可以理解为第几个dentry
 * 
 * @param offset 第几个目录项？
 * @param fi 可忽略
 * @return int 0成功，否则失败
 */
int gfs_readdir(const char * path, void * buf, fuse_fill_dir_t filler, off_t offset,
			    		 struct fuse_file_info * fi) {
    /* TODO: 解析路径，获取目录的Inode，并读取目录项，利用filler填充到buf，可参考/fs/simplefs/sfs.c的Gfs_readdir()函数实现 */
    return 0;
}

/**
 * @brief 创建文件
 * 
 * @param path 相对于挂载点的路径
 * @param mode 创建文件的模式，可忽略
 * @param dev 设备类型，可忽略
 * @return int 0成功，否则失败
 */
int gfs_mknod(const char* path, mode_t mode, dev_t dev) {
	/* TODO: 解析路径，并创建相应的文件 */
	return 0;
}

/**
 * @brief 修改时间，为了不让touch报错 
 * 
 * @param path 相对于挂载点的路径
 * @param tv 实践
 * @return int 0成功，否则失败
 */
int gfs_utimens(const char* path, const struct timespec tv[2]) {
	(void)path;
	return 0;
}
/******************************************************************************
* SECTION: 选做函数实现
*******************************************************************************/
/**
 * @brief 写入文件
 * 
 * @param path 相对于挂载点的路径
 * @param buf 写入的内容
 * @param size 写入的字节数
 * @param offset 相对文件的偏移
 * @param fi 可忽略
 * @return int 写入大小
 */
int gfs_write(const char* path, const char* buf, size_t size, off_t offset,
		        struct fuse_file_info* fi) {
	/* 选做 */
	return size;
}

/**
 * @brief 读取文件
 * 
 * @param path 相对于挂载点的路径
 * @param buf 读取的内容
 * @param size 读取的字节数
 * @param offset 相对文件的偏移
 * @param fi 可忽略
 * @return int 读取大小
 */
int gfs_read(const char* path, char* buf, size_t size, off_t offset,
		       struct fuse_file_info* fi) {
	/* 选做 */
	return size;			   
}

/**
 * @brief 删除文件
 * 
 * @param path 相对于挂载点的路径
 * @return int 0成功，否则失败
 */
int gfs_unlink(const char* path) {
	/* 选做 */
	return 0;
}

/**
 * @brief 删除目录
 * 
 * 一个可能的删除目录操作如下：
 * rm ./tests/mnt/j/ -r
 *  1) Step 1. rm ./tests/mnt/j/j
 *  2) Step 2. rm ./tests/mnt/j
 * 即，先删除最深层的文件，再删除目录文件本身
 * 
 * @param path 相对于挂载点的路径
 * @return int 0成功，否则失败
 */
int gfs_rmdir(const char* path) {
	/* 选做 */
	return 0;
}

/**
 * @brief 重命名文件 
 * 
 * @param from 源文件路径
 * @param to 目标文件路径
 * @return int 0成功，否则失败
 */
int gfs_rename(const char* from, const char* to) {
	/* 选做 */
	return 0;
}

/**
 * @brief 打开文件，可以在这里维护fi的信息，例如，fi->fh可以理解为一个64位指针，可以把自己想保存的数据结构
 * 保存在fh中
 * 
 * @param path 相对于挂载点的路径
 * @param fi 文件信息
 * @return int 0成功，否则失败
 */
int gfs_open(const char* path, struct fuse_file_info* fi) {
	/* 选做 */
	return 0;
}

/**
 * @brief 打开目录文件
 * 
 * @param path 相对于挂载点的路径
 * @param fi 文件信息
 * @return int 0成功，否则失败
 */
int gfs_opendir(const char* path, struct fuse_file_info* fi) {
	/* 选做 */
	return 0;
}

/**
 * @brief 改变文件大小
 * 
 * @param path 相对于挂载点的路径
 * @param offset 改变后文件大小
 * @return int 0成功，否则失败
 */
int gfs_truncate(const char* path, off_t offset) {
	/* 选做 */
	return 0;
}


/**
 * @brief 访问文件，因为读写文件时需要查看权限
 * 
 * @param path 相对于挂载点的路径
 * @param type 访问类别
 * R_OK: Test for read permission. 
 * W_OK: Test for write permission.
 * X_OK: Test for execute permission.
 * F_OK: Test for existence. 
 * 
 * @return int 0成功，否则失败
 */
int gfs_access(const char* path, int type) {
	/* 选做: 解析路径，判断是否存在 */
	return 0;
}	
/******************************************************************************
* SECTION: FUSE入口
*******************************************************************************/
int main(int argc, char **argv)
{
    int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	gfs_options.device = strdup("TODO: 这里填写你的ddriver设备路径");

	if (fuse_opt_parse(&args, &gfs_options, option_spec, NULL) == -1)
		return -1;
	
	ret = fuse_main(args.argc, args.argv, &operations, NULL);
	fuse_opt_free_args(&args);
	return ret;
}