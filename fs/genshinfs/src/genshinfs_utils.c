#include "../include/genshinfs.h"

extern struct gfs_super      gfs_super; 
extern struct custom_options gfs_options;

/**
 * @brief 
 * 
 * @param inode 
 * @param dir [0...]
 * @return struct gfs_dentry* 
 */
struct gfs_dentry* gfs_get_dentry(struct gfs_inode * inode, int dir) {
    struct gfs_dentry* dentry_cursor = inode->dentrys;
    int    cnt = 0;
    while (dentry_cursor)
    {
        if (dir == cnt) {
            return dentry_cursor;
        }
        cnt++;
        dentry_cursor = dentry_cursor->brother;
    }
    return NULL;
}

/**
 * @brief 获取文件名
 * 
 * @param path 
 * @return char* 
 */
char* gfs_get_fname(const char* path) {
    char ch = '/';
    char *q = strrchr(path, ch) + 1;
    return q;
}

/**
 * @brief 计算路径的层级
 * exm: /av/c/d/f
 * -> lvl = 4
 * @param path 
 * @return int 
 */
int gfs_calc_lvl(const char * path) {
    // char* path_cpy = (char *)malloc(strlen(path));
    // strcpy(path_cpy, path);
    char* str = path;
    int   lvl = 0;
    if (strcmp(path, "/") == 0) {
        return lvl;
    }
    while (*str != NULL) {
        if (*str == '/') {
            lvl++;
        }
        str++;
    }
    return lvl;
}

/**
 * @brief  查找返回目标路径的dentry
 * path: /qwe/ad  total_lvl = 2,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry 
 *      3) find qwe's inode     lvl = 2
 *      4) find ad's dentry
 *
 * path: /qwe     total_lvl = 1,
 *      1) find /'s inode       lvl = 1
 *      2) find qwe's dentry
 * 
 * @param path 
 * @return struct gfs_inode* 
 */
struct gfs_dentry* gfs_lookup(const char * path, boolean* is_find, boolean* is_root) {
    struct gfs_dentry* dentry_cursor = gfs_super.root_dentry;
    struct gfs_dentry* dentry_ret = NULL;
    struct gfs_inode*  inode; 
    int   total_lvl = gfs_calc_lvl(path);
    int   lvl = 0;
    boolean is_hit;
    char* fname = NULL;
    char* path_cpy = (char*)malloc(sizeof(path));
    *is_root = FALSE;
    strcpy(path_cpy, path);

    if (total_lvl == 0) {                           /* 根目录 */
        *is_find = TRUE;
        *is_root = TRUE;
        dentry_ret = gfs_super.root_dentry;
    }
    fname = strtok(path_cpy, "/");       
    while (fname)
    {   
        lvl++;
        if (dentry_cursor->inode == NULL) {           /* Cache机制 */
            gfs_read_inode(dentry_cursor, dentry_cursor->ino);
        }

        inode = dentry_cursor->inode;

        if (GFS_IS_REG(inode) && lvl < total_lvl) {
            GFS_DBG("[%s] not a dir\n", __func__);
            dentry_ret = inode->dentry;
            break;
        }
        if (GFS_IS_DIR(inode)) {
            dentry_cursor = inode->dentrys;
            is_hit        = FALSE;

            while (dentry_cursor)
            {
                if (memcmp(dentry_cursor->fname, fname, strlen(fname)) == 0) {
                    is_hit = TRUE;
                    break;
                }
                dentry_cursor = dentry_cursor->brother;
            }
            
            if (!is_hit) {
                *is_find = FALSE;
                GFS_DBG("[%s] not found %s\n", __func__, fname);
                dentry_ret = inode->dentry;
                break;
            }

            if (is_hit && lvl == total_lvl) {
                *is_find = TRUE;
                dentry_ret = dentry_cursor;
                break;
            }
        }
        fname = strtok(NULL, "/"); 
    }

    if (dentry_ret->inode == NULL) {
        dentry_ret->inode = gfs_read_inode(dentry_ret, dentry_ret->ino);
    }
    
    return dentry_ret;
}

/**
 * @brief 驱动读
 * 
 * @param offset 
 * @param out_content 
 * @param size 
 * @return int 
 */
int gfs_driver_read(int offset, uint8_t *out_content, int size) {
    int      offset_aligned = GFS_ROUND_DOWN(offset, GFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = GFS_ROUND_UP((size + bias), GFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    // lseek(GFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(GFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // read(GFS_DRIVER(), cur, GFS_IO_SZ());
        ddriver_read(GFS_DRIVER(), cur, GFS_IO_SZ());
        cur          += GFS_IO_SZ();
        size_aligned -= GFS_IO_SZ();   
    }
    memcpy(out_content, temp_content + bias, size);
    free(temp_content);
    return GFS_ERROR_NONE;
}

/**
 * @brief 驱动写
 * 
 * @param offset 
 * @param in_content 
 * @param size 
 * @return int 
 */
int gfs_driver_write(int offset, uint8_t *in_content, int size) {
    int      offset_aligned = GFS_ROUND_DOWN(offset, GFS_IO_SZ());
    int      bias           = offset - offset_aligned;
    int      size_aligned   = GFS_ROUND_UP((size + bias), GFS_IO_SZ());
    uint8_t* temp_content   = (uint8_t*)malloc(size_aligned);
    uint8_t* cur            = temp_content;
    gfs_driver_read(offset_aligned, temp_content, size_aligned);
    memcpy(temp_content + bias, in_content, size);
    
    // lseek(GFS_DRIVER(), offset_aligned, SEEK_SET);
    ddriver_seek(GFS_DRIVER(), offset_aligned, SEEK_SET);
    while (size_aligned != 0)
    {
        // write(GFS_DRIVER(), cur, GFS_IO_SZ());
        ddriver_write(GFS_DRIVER(), cur, GFS_IO_SZ());
        cur          += GFS_IO_SZ();
        size_aligned -= GFS_IO_SZ();   
    }

    free(temp_content);
    return GFS_ERROR_NONE;
}

/**
 * @brief 分配一个inode，占用位图
 * 
 * @param dentry 该dentry指向分配的inode
 * @return gfs_inode
 */
struct gfs_inode* gfs_alloc_inode(struct gfs_dentry * dentry) {
    struct gfs_inode* inode;
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int ino_cursor  = 0;
    boolean is_find_free_entry = FALSE;

    for (byte_cursor = 0; byte_cursor < GFS_ALL_BLKS_SZ(gfs_super.map_inode_blks); 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((gfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                gfs_super.map_inode[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            ino_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry || ino_cursor >= gfs_super.inode_max_num)
        return -GFS_ERROR_NOSPACE;

    inode = (struct gfs_inode*)malloc(sizeof(struct gfs_inode));
    inode->ino  = ino_cursor; 
    inode->size = 0;

    /* dentry指向inode */
    dentry->inode = inode;
    dentry->ino   = inode->ino;
    
    /* inode指回dentry */
    inode->dentry = dentry;
    
    inode->dir_cnt = 0;
    inode->dentrys = NULL;
    
    if (GFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(GFS_ALL_BLKS_SZ(GFS_DATA_PER_FILE));
    }

    return inode;
}

/**
 * @brief 将内存inode及其下方结构全部刷回磁盘
 * 
 * @param inode 
 * @return int 
 */
int gfs_sync_inode(struct gfs_inode * inode) {
    struct gfs_inode_d  inode_d;
    struct gfs_dentry*  dentry_cursor;
    struct gfs_dentry_d dentry_d;
    int ino             = inode->ino;
    inode_d.ino         = ino;
    inode_d.size        = inode->size;
    // memcpy(inode_d.target_path, inode->target_path, GFS_MAX_FILE_NAME);
    inode_d.ftype       = inode->dentry->ftype;
    inode_d.dir_cnt     = inode->dir_cnt;
    for (int i = 0; i < inode->size; i++)
    {
        inode_d.blocks_pointer[i] = inode->blocks_pointer[i];
    }
    int offset;
    
    // 写入inode
    if (gfs_driver_write(GFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                     sizeof(struct gfs_inode_d)) != GFS_ERROR_NONE) {
        GFS_DBG("[%s] io error\n", __func__);
        return -GFS_ERROR_IO;
    }

    // 写入data
    if  (gfs_driver_write(GFS_DATA_OFS(ino), (uint8_t *)&inode_d,
                            sizeof(struct gfs_inode_d)) != GFS_ERROR_NONE) {
        GFS_DBG("[%s] io error\n", __func__);
        return -GFS_ERROR_IO;
    }
                                                      /* Cycle 1: 写 INODE */
                                                      /* Cycle 2: 写 数据 */
    if (GFS_IS_DIR(inode)) {                          
        dentry_cursor = inode->dentrys;
        offset        = GFS_DATA_OFS(ino);
        while (dentry_cursor != NULL)
        {
            memcpy(dentry_d.fname, dentry_cursor->fname, GFS_MAX_FILE_NAME);
            dentry_d.ftype = dentry_cursor->ftype;
            dentry_d.ino = dentry_cursor->ino;
            if (gfs_driver_write(offset, (uint8_t *)&dentry_d, 
                                 sizeof(struct gfs_dentry_d)) != GFS_ERROR_NONE) {
                GFS_DBG("[%s] io error\n", __func__);
                return -GFS_ERROR_IO;                     
            }
            
            if (dentry_cursor->inode != NULL) {
                gfs_sync_inode(dentry_cursor->inode);
            }

            dentry_cursor = dentry_cursor->brother;
            offset += sizeof(struct gfs_dentry_d);
        }
    }
    else if (GFS_IS_REG(inode)) {
        if (gfs_driver_write(GFS_DATA_OFS(ino), inode->data, 
                             GFS_ALL_BLKS_SZ(GFS_DATA_PER_FILE)) != GFS_ERROR_NONE) {
            GFS_DBG("[%s] io error\n", __func__);
            return -GFS_ERROR_IO;
        }
    }
    return GFS_ERROR_NONE;
}

/**
 * @brief 为一个inode分配dentry，采用头插法
 * 
 * @param inode 
 * @param dentry 
 * @return int 
 */
int gfs_alloc_dentry(struct gfs_inode* inode, struct gfs_dentry* dentry) {
    int byte_cursor = 0; 
    int bit_cursor  = 0; 
    int datam_cursor  = 0;
    boolean is_find_free_entry = FALSE;

    for (byte_cursor = 0; byte_cursor < GFS_ALL_BLKS_SZ(gfs_super.map_inode_blks); 
         byte_cursor++)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            if((gfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) == 0) {    
                                                      /* 当前ino_cursor位置空闲 */
                gfs_super.map_data[byte_cursor] |= (0x1 << bit_cursor);
                is_find_free_entry = TRUE;           
                break;
            }
            datam_cursor++;
        }
        if (is_find_free_entry) {
            break;
        }
    }

    if (!is_find_free_entry || datam_cursor >= gfs_super.data_max_num)
        return -GFS_ERROR_NOSPACE;

    if (inode->size < GFS_DATA_PER_FILE)
        inode->blocks_pointer[inode->size++] = datam_cursor;
    else
        return -GFS_ERROR_NOSPACE;

    if (inode->dentrys == NULL) {
        inode->dentrys = dentry;
    }
    else {
        dentry->brother = inode->dentrys;
        inode->dentrys = dentry;
    }
    inode->dir_cnt++;
    return inode->dir_cnt;
}

/**
 * @brief 
 * 
 * @param dentry dentry指向ino，读取该inode
 * @param ino inode唯一编号
 * @return struct gfs_inode* 
 */
struct gfs_inode* gfs_read_inode(struct gfs_dentry * dentry, int ino) {
    struct gfs_inode* inode = (struct gfs_inode*)malloc(sizeof(struct gfs_inode));
    struct gfs_inode_d inode_d;
    struct gfs_dentry* sub_dentry;
    struct gfs_dentry_d dentry_d;
    int    dir_cnt = 0, i;
    if (gfs_driver_read(GFS_INO_OFS(ino), (uint8_t *)&inode_d, 
                        sizeof(struct gfs_inode_d)) != GFS_ERROR_NONE) {
        GFS_DBG("[%s] io error\n", __func__);
        return NULL;                    
    }
    inode->dir_cnt = 0;
    inode->ino = inode_d.ino;
    inode->size = inode_d.size;
    for (i = 0; i < inode->size; i++)
    {
        inode->blocks_pointer[i] = inode_d.blocks_pointer[i];
    }
    // memcpy(inode->target_path, inode_d.target_path, GFS_MAX_FILE_NAME);
    inode->dentry = dentry;
    inode->dentrys = NULL;
    if (GFS_IS_DIR(inode)) {
        dir_cnt = inode_d.dir_cnt;
        for (i = 0; i < dir_cnt; i++)
        {
            if (gfs_driver_read(GFS_DATA_OFS(ino) + (inode->blocks_pointer[i] * sizeof(struct gfs_dentry_d)), 
                                (uint8_t *)&dentry_d, 
                                sizeof(struct gfs_dentry_d)) != GFS_ERROR_NONE) {
                GFS_DBG("[%s] io error\n", __func__);
                return NULL;                    
            }
            sub_dentry = new_dentry(dentry_d.fname, dentry_d.ftype);
            sub_dentry->parent = inode->dentry;
            sub_dentry->ino    = dentry_d.ino; 
            gfs_alloc_dentry(inode, sub_dentry);
        }
    }
    else if (GFS_IS_REG(inode)) {
        inode->data = (uint8_t *)malloc(GFS_ALL_BLKS_SZ(GFS_DATA_PER_FILE));
        if (gfs_driver_read(GFS_DATA_OFS(ino), (uint8_t *)inode->data, 
                            GFS_ALL_BLKS_SZ(inode->size)) != GFS_ERROR_NONE) {
            GFS_DBG("[%s] io error\n", __func__);
            return NULL;                    
        }
    }
    return inode;
}

/**
 * @brief 挂载genshinfs, Layout 如下
 * 
 * Layout
 * | Super | Inode Map | Data Map | Inodes | Data |
 * 
 * BLK_SZ = 2 * IO_SZ
 * 
 * 每23个Inode占用一个Blk
 * @param options 
 * @return int 
 */
int gfs_mount(struct custom_options options){
    int                 ret = GFS_ERROR_NONE;
    int                 driver_fd;
    struct gfs_super_d  gfs_super_d; 
    struct gfs_dentry*  root_dentry;
    struct gfs_inode*   root_inode;

    int                 inode_num;
    int                 inode_blks;
    int                 map_inode_blks;
    int                 data_num;
    int                 map_data_blks;
    
    int                 super_blks;
    boolean             is_init = FALSE;

    gfs_super.is_mounted = FALSE;

    // driver_fd = open(options.device, O_RDWR);
    driver_fd = ddriver_open(options.device);

    if (driver_fd < 0) {
        return driver_fd;
    }

    gfs_super.driver_fd = driver_fd;
    ddriver_ioctl(GFS_DRIVER(), IOC_REQ_DEVICE_SIZE,  &gfs_super.disk_size);
    ddriver_ioctl(GFS_DRIVER(), IOC_REQ_DEVICE_IO_SZ, &gfs_super.io_size);
    
    root_dentry = new_dentry("/", GFS_DIR);

    if (gfs_driver_read(GFS_SUPER_OFS, (uint8_t *)(&gfs_super_d), 
                        sizeof(struct gfs_super_d)) != GFS_ERROR_NONE) {
        return -GFS_ERROR_IO;
    }   

    GFS_DBG("读取super_d成功\n");
                                                      /* 读取super */
    if (gfs_super_d.magic_num != GFS_MAGIC_NUM) {     /* 幻数无 */
                                                      /* 估算各部分大小 */
        GFS_DBG("发现无magic number，开始初始化布局\n");
        int total_blks = GFS_ROUND_DOWN(GFS_DISK_SZ(), GFS_BLK_SZ()) / GFS_BLK_SZ();
        super_blks = 1;
        map_inode_blks = 1;
        map_data_blks = 1;
        inode_num = 581;
        inode_blks = 26;
        data_num = inode_num * GFS_DATA_PER_FILE;


                                                      /* 布局layout */
        gfs_super_d.inode_max_num = inode_num; 
        gfs_super_d.data_max_num  = data_num;
        gfs_super_d.inode_blks    = inode_blks;
        
        gfs_super_d.block_size = GFS_BLK_SZ();
        gfs_super_d.block_num  = total_blks;

        gfs_super_d.map_inode_offset = GFS_SUPER_OFS + GFS_ALL_BLKS_SZ(super_blks);
        gfs_super_d.map_data_offset  = gfs_super_d.map_inode_offset + GFS_ALL_BLKS_SZ(map_inode_blks);
        gfs_super_d.inode_offset     = gfs_super_d.map_data_offset + GFS_ALL_BLKS_SZ(map_data_blks);
        gfs_super_d.data_offset      = gfs_super_d.inode_offset + GFS_ALL_BLKS_SZ(inode_blks);

        gfs_super_d.map_inode_blks  = map_inode_blks;
        gfs_super_d.map_data_blks   = map_data_blks;
        gfs_super_d.usage    = 0;
        is_init = TRUE;
    }  
    /* 建立 in-memory 结构 */
    
    gfs_super.block_size = gfs_super_d.block_size;
    gfs_super.block_num  = gfs_super_d.block_num;
    gfs_super.inode_blks = gfs_super_d.inode_blks;

    gfs_super.map_inode = (uint8_t *)malloc(GFS_ALL_BLKS_SZ(gfs_super_d.map_inode_blks));
    gfs_super.map_data  = (uint8_t *)malloc(GFS_ALL_BLKS_SZ(gfs_super_d.map_data_blks));
    gfs_super.map_inode_blks = gfs_super_d.map_inode_blks;
    gfs_super.map_data_blks  = gfs_super_d.map_data_blks;
    gfs_super.map_inode_offset = gfs_super_d.map_inode_offset;
    gfs_super.map_data_offset  = gfs_super_d.map_data_offset;
    
    gfs_super.inode_offset     = gfs_super_d.inode_offset;
    gfs_super.inode_blks       = gfs_super_d.inode_blks;
    gfs_super.data_offset = gfs_super_d.data_offset;

    gfs_super.inode_max_num = gfs_super_d.inode_max_num;
    gfs_super.data_max_num  = gfs_super_d.data_max_num;

    GFS_DBG("读取inode map\n");
    // 读取inode map
    if (gfs_driver_read(gfs_super_d.map_inode_offset, (uint8_t *)(gfs_super.map_inode), 
                        GFS_ALL_BLKS_SZ(gfs_super_d.map_inode_blks)) != GFS_ERROR_NONE) {
        return -GFS_ERROR_IO;
    }
    gfs_dump_inode_map();


    GFS_DBG("读取data map\n");
    // 读取data map
    if (gfs_driver_read(gfs_super_d.map_data_offset, (uint8_t *)(gfs_super.map_data), 
                        GFS_ALL_BLKS_SZ(gfs_super_d.map_data_blks)) != GFS_ERROR_NONE) {
        return -GFS_ERROR_IO;
    }
    gfs_dump_data_map();

    if (is_init) {                                    /* 分配根节点 */
        GFS_DBG("检测到第一次读盘，初始化根节点\n");
        root_inode = gfs_alloc_inode(root_dentry);
        GFS_DBG("分配根节点成功，尝试将根写回\n");
        gfs_sync_inode(root_inode);
    }
    
    GFS_DBG("读取根节点\n");
    root_inode            = gfs_read_inode(root_dentry, GFS_ROOT_INO);
    root_dentry->inode    = root_inode;
    gfs_super.root_dentry = root_dentry;
    gfs_super.is_mounted  = TRUE;

    gfs_dump_super();
    return ret;
}

/**
 * @brief 
 * 
 * @return int 
 */
int gfs_umount() {
    struct gfs_super_d  gfs_super_d; 

    if (!gfs_super.is_mounted) {
        return GFS_ERROR_NONE;
    }

    gfs_sync_inode(gfs_super.root_dentry->inode);     /* 从根节点向下刷写节点 */
                                                    
    gfs_super_d.magic_num           = GFS_MAGIC_NUM;
    gfs_super_d.block_size          = gfs_super.block_size;
    gfs_super_d.usage               = gfs_super.usage;
    
    gfs_super_d.map_inode_blks      = gfs_super.map_inode_blks;
    gfs_super_d.map_inode_offset    = gfs_super.map_inode_offset;
    gfs_super_d.map_data_blks       = gfs_super.map_data_blks;
    gfs_super_d.map_data_offset     = gfs_super.map_data_offset;

    gfs_super_d.inode_offset        = gfs_super.inode_offset;
    gfs_super_d.inode_blks          = gfs_super.inode_blks;
    gfs_super_d.data_offset         = gfs_super.data_offset;

    gfs_super_d.inode_max_num       = gfs_super.inode_max_num;
    gfs_super_d.data_max_num        = gfs_super.data_max_num;

    if (gfs_driver_write(GFS_SUPER_OFS, (uint8_t *)&gfs_super_d, 
                     sizeof(struct gfs_super_d)) != GFS_ERROR_NONE) {
        return -GFS_ERROR_IO;
    }

    // 写入inode map
    if (gfs_driver_write(gfs_super_d.map_inode_offset, (uint8_t *)(gfs_super.map_inode), 
                         GFS_ALL_BLKS_SZ(gfs_super_d.map_inode_blks)) != GFS_ERROR_NONE) {
        return -GFS_ERROR_IO;
    }

    // 写入data map
    if  (gfs_driver_write(gfs_super_d.map_data_offset, (uint8_t *)(gfs_super.map_data), 
                          GFS_ALL_BLKS_SZ(gfs_super_d.map_data_blks)) != GFS_ERROR_NONE) {
        return -GFS_ERROR_IO;
    }

    free(gfs_super.map_inode);
    ddriver_close(GFS_DRIVER());

    return GFS_ERROR_NONE;
}