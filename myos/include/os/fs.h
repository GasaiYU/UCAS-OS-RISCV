#ifndef _FS_H
#define _FS_H

#include <type.h>

/*Sec num*/
#define MAGIC    0x20190625
#define BS       (1 << 12) /*Block  size  4KB*/
#define SS       (1 << 9 ) /*Sector size 512B*/
#define FS_SIZE  (1 << 20) /*FS have 1M  SS*/
#define FS_START (1 << 20) /*FS start block num*/
#define ISZIE    128
#define DSIZE    32

#define SB_SD_OFFSET     0  /*1 sector*/
#define BMAP_SD_OFFST    1  /*32 sector(512MB)*/
#define IMAP_SD_OFFSET  33  /*1 sector(512*512B)*/
#define INODE_SD_OFFSET 34  /*512 sector*/
#define DATA_SD_OFFSET  546 /*almost 512MB*/

//BA(pa)
#define SB_MEM_ADDR       0x5d000000
#define BMAP_MEM_ADDR     0x5d000200
#define IMAP_MEM_ADDR     0x5d004200
#define INODE_MEM_ADDR    0x5d004400
#define LEV1_MEM_ADDR     0x5d044400
#define DATA_MEM_ADDR     0x5d048400

//Inode
#define DPTR 10
#define MAX_FILE_NAME 24
#define DIR  0
#define FILE 1
#define R_A  1
#define W_A  2
#define RW_A 3

#define MAXFD 30

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define IPS (512/sizeof(inode_t))

typedef struct superblock{
    /*judge if this fs exit*/
    u32 magic;
    /*fs info*/
    u32 fs_size;
    u32 fs_start;
    /*block map info*/
    u32 block_map_num;
    u32 block_map_start;
    /*inode map info*/
    u32 inode_map_num;
    u32 inode_map_start;
    /*data block info Note: bmap 1 symbolize 1 block*/
    u32 data_block_num;
    u32 data_block_start;
    /*inode info Note: imap 1 symbolize 1 sector*/
    u32 inode_num;
    u32 inode_start;
    /*size*/
    u32 isize;
    u32 dsize
}superblock_t;

typedef struct inode
{
    u8 mode;        //File/Dir
    u8 access;      //R/W/EXC
    u16 nlinks;     //Link num
    u32 size;       //Size
    u32 ino;        //inumber
    u32 direct_ptr[DPTR];
    u32 lev1_ptr[3];
    u32 lev2_ptr[2];
    u32 lev3_ptr;
    u32 usize;      //Used size
    u64 mtime;      //Modified time
    u64 stime;      //Start times
    u64 nop[4];     //TO make aligned 128
}inode_t;

typedef struct dentry {
    u32 mode;                  
    u32 ino;                  
    char name[MAX_FILE_NAME];
}dentry_t;

typedef struct fd
{
    u32 ino;
    u32 r_ptr;
    u32 w_ptr;
    u8  access;
}fd_t;


/*syscall*/
void do_mkfs(void);
void do_statfs(void);
void do_mkdir(char *name);
void do_cd(char *path);
void do_ls(char *path);
void do_rmdir(char *name);
void do_touch(char *name);

int do_fopen(char *name, u8 mode);
int do_fread(int fd, char *buff, int size);
void do_fclose(int fd);
int do_fwrite(int fd, char *buff, int size);
void do_cat(char *name);
void do_lseek(int fd, int offset, int whence);
void do_ln(char *src, char *dst);
void do_rm(char *name);
void do_ls_l();
#endif