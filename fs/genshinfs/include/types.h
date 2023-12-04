#ifndef _TYPES_H_
#define _TYPES_H_


/******************************************************************************
* SECTION: Type def
*******************************************************************************/
typedef int          boolean;

typedef enum gfs_file_type {
    GFS_REG_FILE,
    GFS_DIR
} GFS_FILE_TYPE;

struct custom_options {
	const char*        device;
};


/******************************************************************************
* SECTION: Macro
*******************************************************************************/
#define TRUE                    1
#define FALSE                   0
#define UINT32_BITS             32
#define UINT8_BITS              8

#define GFS_MAGIC_NUM           0x11451419
#define GFS_SUPER_OFS           0
#define GFS_ROOT_INO            0

#define GFS_ERROR_NONE          0
#define GFS_ERROR_ACCESS        EACCES
#define GFS_ERROR_SEEK          ESPIPE     
#define GFS_ERROR_ISDIR         EISDIR
#define GFS_ERROR_NOSPACE       ENOSPC
#define GFS_ERROR_EXISTS        EEXIST
#define GFS_ERROR_NOTFOUND      ENOENT
#define GFS_ERROR_UNSUPPORTED   ENXIO
#define GFS_ERROR_IO            EIO     /* Error Input/Output */
#define GFS_ERROR_INVAL         EINVAL  /* Invalid Args */

#define GFS_MAX_FILE_NAME       128
#define GFS_IOS_PER_BLK         2
#define GFS_INODE_PER_FILE      1
#define GFS_DATA_PER_FILE       7
#define GFS_DEFAULT_PERM        0777   /* 全权限打开 */

#define GFS_IOC_MAGIC           'S'
#define GFS_IOC_SEEK            _IO(GFS_IOC_MAGIC, 0)

#define GFS_FLAG_BUF_DIRTY      0x1
#define GFS_FLAG_BUF_OCCUPY     0x2


/******************************************************************************
* SECTION: Macro Function
*******************************************************************************/
#define GFS_IO_SZ()                     (gfs_super.io_size)
#define GFS_BLK_SZ()                    (GFS_IO_SZ() * GFS_IOS_PER_BLK)
#define GFS_DISK_SZ()                   (gfs_super.disk_size)
#define GFS_DRIVER()                    (gfs_super.driver_fd)

#define GFS_ROUND_DOWN(value, round)    (value % round == 0 ? value : (value / round) * round)
#define GFS_ROUND_UP(value, round)      (value % round == 0 ? value : (value / round + 1) * round)

#define GFS_ALL_BLKS_SZ(blks)               (blks * GFS_BLK_SZ())
#define GFS_ASSIGN_FNAME(pgfs_dentry, _fname)   memcpy(pgfs_dentry->fname, _fname, strlen(_fname))

#define GFS_INO_OFS(ino)                (gfs_super.inode_offset + ino * GFS_ALL_BLKS_SZ(GFS_INODE_PER_FILE))
#define GFS_DATA_OFS(ino)               (gfs_super.data_offset + ino * GFS_ALL_BLKS_SZ(GFS_INODE_PER_FILE))

#define GFS_IS_DIR(pinode)              (pinode->dentry->ftype == GFS_DIR)
#define GFS_IS_REG(pinode)              (pinode->dentry->ftype == GFS_REG_FILE)


/******************************************************************************
* SECTION: FS Specific Structure - In memory structure
*******************************************************************************/


struct gfs_inode
{
    int                ino;                           /* 在inode位图中的下标 */
    
    // 文件属性
    int                size;                          /* 文件已占用空间 */
    GFS_FILE_TYPE      ftype;                         /* 文件类型，目录或文件 */
    
    // 数据块索引（每个文件有x个数据块）
    int                blocks_pointer[GFS_DATA_PER_FILE];

    // 如果是目录类型，下有几个目录项
    int                dir_cnt;

    struct gfs_dentry* dentry;                        /* 指向该inode的dentry */
    struct gfs_dentry* dentrys;                       /* 所有目录项 */
    uint8_t*           data;    
};  

struct gfs_dentry
{
    char               fname[GFS_MAX_FILE_NAME];
    GFS_FILE_TYPE      ftype;
    int                ino;                           /* 指向的ino号 */

    struct gfs_dentry* parent;                        /* 父亲Inode的dentry */
    struct gfs_dentry* brother;                       /* 兄弟 */
    struct gfs_inode*  inode;                         /* 指向inode */
};  

struct gfs_super 
{
    int io_size; // 物理块大小
    int driver_fd;
    int disk_size;
    int usage;

    // 逻辑块信息
    int block_size;
    int block_num;

    int map_inode_offset; // inode位图于磁盘中的偏移
    int map_inode_blks;   // inode位图占用的块数

    int map_data_offset;   // 数据块位图于磁盘中的偏移
    int map_data_blks;     // 数据块位图占用的块数

    int inode_offset;     // inode区于磁盘中的偏移
    int inode_blks;       // inode区占用的块数

    int data_offset;      // 数据区于磁盘中的偏移

    int inode_max_num;    // inode最大数量
    int data_max_num;    // 数据块最大数量

    struct gfs_dentry* root_dentry;     // 根目录
    boolean is_mounted;   // 是否已挂载
    uint8_t* map_inode;   // inode位图
    uint8_t* map_data;    // 数据块位图

};

static inline struct gfs_dentry* new_dentry(char * fname, GFS_FILE_TYPE ftype) {
    struct gfs_dentry * dentry = (struct gfs_dentry *)malloc(sizeof(struct gfs_dentry));
    memset(dentry, 0, sizeof(struct gfs_dentry));
    GFS_ASSIGN_FNAME(dentry, fname);
    dentry->ftype   = ftype;
    dentry->ino     = -1;
    dentry->inode   = NULL;
    dentry->parent  = NULL;
    dentry->brother = NULL;
    return dentry;                                            
}

/******************************************************************************
* SECTION: FS Specific Structure - Disk structure
*******************************************************************************/

struct gfs_super_d {
    uint32_t magic_num;
    int usage;
    
    // 逻辑块信息
    int block_size;
    int block_num;


    int map_inode_offset; // inode位图于磁盘中的偏移
    int map_inode_blks;   // inode位图占用的块数

    int map_data_offset;   // 数据块位图于磁盘中的偏移
    int map_data_blks;     // 数据块位图占用的块数

    int inode_offset;     // inode区于磁盘中的偏移
    int inode_blks;       // inode区占用的块数

    int data_offset;      // 数据区于磁盘中的偏移

    int inode_max_num;    // inode最大数量
    int data_max_num;    // 数据块最大数量
};

struct gfs_inode_d
{
    int                ino;                           /* 在inode位图中的下标 */
    
    // 文件属性
    int                size;                          /* 文件已占用空间 */
    GFS_FILE_TYPE      ftype;                         /* 文件类型，目录或文件 */
    
    // 数据块索引（每个文件有x个数据块）
    int                blocks_pointer[GFS_DATA_PER_FILE];

    // 如果是目录类型，下有几个目录项
    int                dir_cnt;
};  

struct gfs_dentry_d
{
    char               fname[GFS_MAX_FILE_NAME];
    GFS_FILE_TYPE      ftype;
    int                ino;                           /* 指向的ino号 */
};  


#endif /* _TYPES_H_ */