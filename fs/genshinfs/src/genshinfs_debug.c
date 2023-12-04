#include "../include/genshinfs.h"

extern struct gfs_super      gfs_super; 
extern struct custom_options gfs_options;

void gfs_dump_inode_map() {
    int byte_cursor = 0;
    int bit_cursor = 0;

    printf("===========inode map===========\n");

    for (byte_cursor = 0; byte_cursor < 32; //GFS_ALL_BLKS_SZ(gfs_super.map_inode_blks); 
         byte_cursor+=4)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_inode[byte_cursor] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_inode[byte_cursor + 1] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_inode[byte_cursor + 2] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_inode[byte_cursor + 3] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\n");
    }
}

void gfs_dump_data_map() {
    int byte_cursor = 0;
    int bit_cursor = 0;

    printf("===========data map===========\n");

    for (byte_cursor = 0; byte_cursor < 32; //GFS_ALL_BLKS_SZ(gfs_super.map_data_blks); 
         byte_cursor+=4)
    {
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_data[byte_cursor] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");

        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_data[byte_cursor + 1] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_data[byte_cursor + 2] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\t");
        
        for (bit_cursor = 0; bit_cursor < UINT8_BITS; bit_cursor++) {
            printf("%d ", (gfs_super.map_data[byte_cursor + 3] & (0x1 << bit_cursor)) >> bit_cursor);   
        }
        printf("\n");
    }
}

void gfs_dump_inode(struct gfs_inode * inode) {
    printf("===========inode (ino: %d)===========\n", inode->ino);
    printf("ftype: %d\n", inode->ftype);
    printf("size: %d\n", inode->size);
}

void gfs_dump_dentry(struct gfs_dentry * dentry) {
    printf("===========dentry (指向ino: %d)===========\n", dentry->ino);
    printf("fname: %s\n", dentry->fname);
    printf("ftype: %d\n", dentry->ftype);
}

void gfs_dump_super() {
    printf("===========super===========\n");
    printf("disk_size: %d\n", gfs_super.disk_size);
    printf("io_size: %d\n", gfs_super.io_size);
    printf("usage: %d\n", gfs_super.usage);
    printf("block_size: %d\n", gfs_super.block_size);
    printf("block_num: %d\n", gfs_super.block_num);
    printf("map_inode_offset: %d\n", gfs_super.map_inode_offset);
    printf("map_inode_blks: %d\n", gfs_super.map_inode_blks);
    printf("map_data_offset: %d\n", gfs_super.map_data_offset);
    printf("map_data_blks: %d\n", gfs_super.map_data_blks);
    printf("inode_offset: %d\n", gfs_super.inode_offset);
    printf("inode_blks: %d\n", gfs_super.inode_blks);
    printf("data_offset: %d\n", gfs_super.data_offset);
    printf("inode_max_num: %d\n", gfs_super.inode_max_num);
    printf("data_max_num: %d\n", gfs_super.data_max_num);
    printf("is_mounted: %d\n", gfs_super.is_mounted);
}