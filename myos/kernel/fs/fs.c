#include <os/list.h>
#include <os/fs.h>
#include <sbi.h>
#include <os/mm.h>
#include <os/time.h>
#include <screen.h>
#include <pgtable.h>
#include <os/stdio.h>
#include <os/string.h>

inode_t *current_inode;
int state = 0;

void clears(uintptr_t mem_addr){
    int i = 0;
    for(i = 0; i < 512; i+=8){
        *((u64*)(mem_addr+i)) = 0;
    }
}

/*Serch bmap and get free data block*/
u32 alloc_data_block(){
    sbi_sd_read(BMAP_MEM_ADDR, 32, FS_START + BMAP_SD_OFFST);
    u8 *bm = (u8*)pa2kva(BMAP_MEM_ADDR);
    u32 fb = 0;
    for(int i = 0; i < 32*512; i++){
        for(int j = 0; j < 8; j++){
            if(!(bm[i] & (1 << j))){
                fb = 8*i + j;
                //Set bit 1
                bm[i] |= (1 << j);
                sbi_sd_write(BMAP_MEM_ADDR, 32, FS_START + BMAP_SD_OFFST);
                return fb;
            }
        }
    }
    return 0;
}

void clear_data_block(u32 block_id){
    sbi_sd_read(BMAP_MEM_ADDR, 32, FS_START + BMAP_SD_OFFST);
    u8 *bm = (u8*)pa2kva(BMAP_MEM_ADDR);
    bm[block_id/8] &= ~(1 << (block_id % 8));
    sbi_sd_write(BMAP_MEM_ADDR, 32, FS_START + BMAP_SD_OFFST);
}

u32 alloc_inode(){
    sbi_sd_read(IMAP_MEM_ADDR, 1, FS_START + IMAP_SD_OFFSET);
    u8 *im = (u8*)(pa2kva(IMAP_MEM_ADDR));
    u32 ret_inode = 0;
    for(int i = 0; i < 512; i++){
        for(int j = 0; j < 8; j++){
            if(!(im[i] & (1 << j))){
                ret_inode = 8*i + j;
                //set bit 1
                im[i] |= (1 << j);
                sbi_sd_write(IMAP_MEM_ADDR, 1, FS_START + IMAP_SD_OFFSET);
                return ret_inode;
            }
        }
    }
}

void clear_inode_map(u32 ino){
    sbi_sd_read(IMAP_MEM_ADDR, 1, FS_START + IMAP_SD_OFFSET);
    u8 *im = (u8*)(pa2kva(IMAP_MEM_ADDR));
    im[ino/8] &= ~(1 << (ino % 8));
    sbi_sd_write(IMAP_MEM_ADDR, 1, FS_START + IMAP_SD_OFFSET); 
}

/*Write a inode into our disk*/
void write_inode(u32 ino){
    sbi_sd_write(INODE_MEM_ADDR + ino*512, 1, FS_START + INODE_SD_OFFSET + ino);
}

/*Init . and .. for dir, 2*32 = 64B*/
void init_dentry(u32 block_num, u32 ino, u32 pino){
    dentry_t *d = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    clears(d);
    d[0].mode = DIR;
    d[0].ino  = ino;
    kstrcpy(d[0].name, ".");

    d[1].mode = DIR;
    d[1].ino  = pino;
    kstrcpy(d[1].name, "..");

    sbi_sd_write(DATA_MEM_ADDR, 8, FS_START + block_num*8);
}

inode_t *get_inode_from_ino(u32 ino){
    sbi_sd_read(INODE_MEM_ADDR + ino*512, 1, FS_START + INODE_SD_OFFSET + ino);
    inode_t *find_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR + ino*512));
    return find_inode;
}

inode_t *lookup(inode_t *parent_dp, char *name, int mode){

    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (parent_dp -> direct_ptr[0])*8);
    dentry_t *d = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    //We can only create a dir in a dir
    /*if(d -> mode != DIR){
        printk(">Error: You can only create a file or dir in a dir");
        return 0;
    }*/
    for(int i = 0; i < parent_dp -> usize; i += 32){
        if((!strcmp(d[i/32].name, name))){
            inode_t *find_inode = get_inode_from_ino(d[i/32].ino);
            return find_inode;
        }
    }
    return NULL;
}

//to find path, we use recurssion
int find  = 0;
int depth = 0;
int find_dir(char *path, int mode){
    depth = 0;
    int i = 0;
    int j = 0;
    if(depth == 0){
        find = 0;
    }
    /*If absolute path*/
    if(path[0] == '/'){
        sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        if(path[1] == '\0'){
            return 1;
        }
        for(i = 0; i < kstrlen(path); i++){
            path[i] = path[i+1];
        }
        path[i] = '\0';
    }
    char head[20];
    char tail[100];
    for(i = 0; i < kstrlen(path) && path[i] != '/'; i++){
        head[i] = path[i];
    }
    head[i++] = '\0';
    for(j = 0; i < kstrlen(path); i++, j++){
        tail[j] = path[i]; 
    }
    tail[j] = '\0';
    
    inode_t *find_inode;
    if((find_inode = lookup(current_inode, head, mode)) != 0){
        depth ++;
        current_inode = find_inode;
        if(tail[0] == '\0'){
            find = 1;
            return 1;
        }
        find_dir(tail, mode);
    }
    if(find  == 1){
        depth --;
        return 1;
    }else{
        depth = 0;
        return 0;
    }
}

//syscall
void do_mkfs(){
    int i;
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    screen_reflush();
    printk("[FS] Start initializing filesystem!\n");

    //Superblock init!
    printk("[FS] Setting superblock...\n");
    clears(sp);
    //fill SP
    sp -> magic    = MAGIC;
    sp -> fs_size  = FS_SIZE;
    sp -> fs_start = FS_START;
    sp -> block_map_num = 32;
    sp -> block_map_start = BMAP_SD_OFFST;
    sp -> inode_map_num = 1;
    sp -> inode_map_start = IMAP_SD_OFFSET;
    sp -> inode_num = 512;
    sp -> inode_start = INODE_SD_OFFSET;
    sp -> data_block_num = 1048576 - 546;
    sp -> data_block_start = DATA_SD_OFFSET;
    sp -> isize = ISZIE;
    sp -> dsize = DSIZE;
    printk("\n");
    printk("[FS] magic: 0x%x\n", sp -> magic);
    printk("[FS] num sector: %d, start sector: %d\n",sp -> fs_size, sp -> fs_start);
    printk("[FS] block map offset: %d(%d)\n", sp -> block_map_start, sp -> block_map_num);
    printk("[FS] inode map offset: %d(%d)\n", sp -> inode_map_start, sp -> inode_map_num);
    printk("[FS] inode offset: %d(%d)\n", sp -> inode_start, sp -> inode_num);
    printk("[FS] data offset: %d(%d)\n",sp -> data_block_start, sp -> data_block_num);
    printk("[FS] inode entry size: %dB, dir entry size: %dB\n", sp -> isize, sp -> dsize);
    printk("IENTRY SIZE:%d\n");

    //set block map
    printk("[FS] Setting block map...\n");
    u8 *bm = (u8*)(pa2kva(BMAP_MEM_ADDR));
    for(i = 0; i < sp -> block_map_num; i++){
        clears(bm + SS*i);
    }
    //Note: (512+2+32)/8 + 1 = 69
    for(i = 0; i < 69; i++){
        bm[i/8] = bm[i/8] | (1 << (i % 8));
    }
    //set inode map
    printk("[FS] Setting inode map...\n");
    u8 *im = (u8*)(pa2kva(IMAP_MEM_ADDR));
    clears(im);
    im[0] |= 1; /*dir*/
    sbi_sd_write(SB_MEM_ADDR, 34/*sp+im+bm*/, FS_START);

    //Setting inode
    printk("[FS] Setting inode...\n");
    inode_t *inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
    inode[0].ino    = 0;
    inode[0].mode   = DIR;
    inode[0].access = RW_A;
    inode[0].nlinks = 0;
    inode[0].size   = 4096;
    inode[0].usize  = 64;       //Two dentry
    inode[0].stime  = get_timer();
    inode[0].mtime  = get_timer();
    inode[0].lev1_ptr[0] = 0;
    inode[0].lev1_ptr[1] = 0;
    inode[0].lev1_ptr[2] = 0;
    inode[0].lev2_ptr[0] = 0;
    inode[0].lev2_ptr[1] = 0;
    inode[0].lev3_ptr = 0;
    inode[0].direct_ptr[0] = alloc_data_block();
    for(i = 1; i < DPTR; i++){
        inode[0].direct_ptr[i] = 0;
    }
    write_inode(inode[0].ino);
    //Init dentry
    init_dentry(inode[0].direct_ptr[0], inode[0].ino, 0);

    printk("Initializing filesystem finished!\n");
    screen_reflush();
    current_inode = inode;
}

void do_statfs(){
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    printk("\n");
    //check sp
    if(sp -> magic != MAGIC){
        printk(">ERROR: No File System!\n");
        return;
    }
    
    int i,j;
    int used_block = 0;
    int used_inode = 0;
    //used block
    sbi_sd_read(BMAP_MEM_ADDR, 32, FS_START + BMAP_SD_OFFST);
    u8 *bm = (u8*)(pa2kva(BMAP_MEM_ADDR));
    for(i = 0; i < 32*512; i++){
        for(j = 0; j < 8; j++){
            used_block += (bm[i] >> j) & 0x1;
        }
    }
    //used inode
    sbi_sd_read(IMAP_MEM_ADDR, 1, FS_START + IMAP_SD_OFFSET);
    u8 *im = (u8*)(pa2kva(IMAP_MEM_ADDR));
    for(i = 0; i < 512; i++){
        for(j = 0; j < 8; j++){
            used_inode += (im[i] >> j) & 0x1;
        }
    }
    //screen_move_cursor(1,1);
    printk("magic: 0x%x\n",sp -> magic);
    printk("used block: %d/%d, start sector: %d(0x%x)\n",used_block, FS_SIZE, FS_START, FS_START);
    printk("inode map offset: %d, occupied sector: %d, used: %d/512\n",sp -> inode_map_start, sp -> inode_map_num, used_inode);
    printk("block map offset: %d, occupied block: %d\n", sp -> block_map_start, sp -> block_map_num);
    printk("inode offset: %d, occupied sector: %d\n", sp -> inode_start, sp -> inode_num);
    printk("data offset: %d, occupied sector: %d\n", sp -> data_block_start, sp -> data_block_num);
    printk("inode entry size: %dB, dir entry size: %dB\n", sp -> isize, sp -> dsize);
}

//mkdir
void do_mkdir(char *name){
    //check if there is fs
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    printk("\n");
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("> Error: Fs does not exit!\n");
        return;
    }
    //Step1: check if this dir exit
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    inode_t *father_dir = current_inode;      //Father
    inode_t *find_dir   = lookup(father_dir, name, DIR);
    if(find_dir != NULL){
        printk("This DIR has already exited\n");
        return;
    }
    //Step 2: alloc an inode and write inode map bit 1
    u32 inode_snum = alloc_inode();
    if(inode_snum == 0){
        printk(">ERROR: No Available Inode!\n");
        return;
    }
    inode_t *new_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR) + inode_snum*512);
    new_inode -> ino    = inode_snum;
    new_inode -> mode   = DIR;
    new_inode -> access = RW_A;
    new_inode -> nlinks = 0;
    new_inode -> size   = 4096;
    new_inode -> usize  = 64; //. & .. dir
    new_inode -> stime = new_inode -> mtime = get_timer();
    new_inode -> direct_ptr[0] = alloc_data_block();  
    for(int i = 1; i < DPTR; i++){
        new_inode -> direct_ptr[i] = 0;
    }
    (new_inode -> lev1_ptr)[0] = 0;
    (new_inode -> lev1_ptr)[1] = 0;
    (new_inode -> lev1_ptr)[2] = 0;
    (new_inode -> lev2_ptr)[0] = 0;
    (new_inode -> lev2_ptr)[1] = 0;
    new_inode -> lev3_ptr = 0;
    write_inode(new_inode -> ino);
    //Create dentry . and ..
    init_dentry(new_inode -> direct_ptr[0], new_inode -> ino, father_dir -> ino);
    //add dentry in its parent dir
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (father_dir -> direct_ptr[0])*8);
    dentry_t *dentry = (dentry_t*)(pa2kva(DATA_MEM_ADDR) + father_dir -> usize); //find unused addr
    dentry -> mode = DIR;
    dentry -> ino  = new_inode -> ino;
    kstrcpy(dentry -> name, name);
    sbi_sd_write(DATA_MEM_ADDR, 8, FS_START + (father_dir -> direct_ptr[0])*8);
    //update father inode
    father_dir -> mtime = get_timer();
    father_dir -> nlinks ++;
    father_dir -> usize += 32;
    write_inode(father_dir -> ino);
    //debugging
    //sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + 69*8);
    //dentry = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    //printk("Dentry[2]:%s",dentry[2].name);
    //printk("Dentry[3]:%s",dentry[3].name);
    //debugging end
    printk("Successfully make a directory!\n");
}



//change directory
void do_cd(char *path){
    //check if there is fs
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("File System does not exit!\n");
        return;
    }

    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }

    inode_t *temp = current_inode;
    if(path[0] != '\0'){
        if(find_dir(path, DIR) == 0 || current_inode -> mode != DIR){
            current_inode = temp;
            printk(">ERROR: Cannot find this directory");
            return;
        }
    }
    if(current_inode -> mode == FILE){
        current_inode = temp;
    }
}

void do_ls(char *path){
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("fs does not exit!\n");
        return;
    }
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    inode_t *temp_inode = current_inode;
    if(path != '\0'){
        int found1 = find_dir(path, DIR);
        if(!found1){
            current_inode = temp_inode;
        }
    }
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (current_inode -> direct_ptr[0])*8);
    dentry_t *d = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    printk("\n");
    for(int i = 0; i < current_inode -> usize; i+=32){
        printk("%s ", d[i/32].name);
    }
    printk("\n");
    current_inode = temp_inode;
}

void do_rmdir(char *name){
    //check fs
    int i;
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("File system does not exit!\n");
        return;
    }
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }

    inode_t *father_inode = current_inode;
    inode_t *find_dir = lookup(father_inode, name, DIR);
    
    //printk("Find_dir  -> ino: %d\n", find_dir -> ino);
    if(find_dir == NULL){
        printk(">ERROR: Cannot find this dir!\n");
        return;
    }
    //clear block map
    clear_data_block(find_dir -> direct_ptr[0]);
    //clear inode map
    clear_inode_map(find_dir -> ino);
    //delete file
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    dentry_t *d = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    int found1 = 0;
    for(i = 0; i < father_inode -> usize; i+= 32){
        if(found1){
            memcpy((u8*)(d - 1), (u8*)d, 32);
        }
        if(find_dir -> ino == d -> ino){
            found1 = 1;
        }
        d++;
    }
    sbi_sd_write(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    //delete all files
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (find_dir -> direct_ptr[0])*8);
    dentry_t *rm_d = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    for(i = 0; i < find_dir -> usize; i+=32){
        if(rm_d[i/32].mode == FILE){
            inode_t *find_file = lookup(father_inode, rm_d[i/32].name, FILE);
            if(find_file != NULL){
                clear_data_block(find_file -> direct_ptr[0]);
                clear_inode_map(find_file -> ino);
            }
        }
    }
    //update father inode
    father_inode -> nlinks --;
    father_inode -> usize -= 32;
    father_inode -> mtime = get_timer();
    write_inode(father_inode -> ino);
    
    printk("\nFinish delete a directory!\n");
}

//File operation
void do_touch(char *name){
    int i = 0;
    //Judge if there is a fs
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }

    //Judge if this file has exited
    inode_t *father_inode = current_inode;
    inode_t *find_file = lookup(father_inode, name, FILE);
    if(find_file != NULL){
        printk("This file has already existed!\n");
        return;
    }
    //alloc inode and 1 data block
    u32 inode_snum = alloc_inode();
    inode_t *new_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR) + inode_snum*512);
    new_inode -> ino = inode_snum;
    new_inode -> mode = FILE;
    new_inode -> access = RW_A;
    new_inode -> nlinks = 1;
    new_inode -> size = 0;
    new_inode -> usize = 0;//different from dir
    new_inode -> stime = new_inode -> mtime = get_timer();
    new_inode -> direct_ptr[0] = alloc_data_block();
    for(int i = 1; i < DPTR; i++){
        new_inode -> direct_ptr[i] = 0;
    }
    (new_inode -> lev1_ptr)[0] = 0;
    (new_inode -> lev1_ptr)[1] = 0;
    (new_inode -> lev1_ptr)[2] = 0;
    (new_inode -> lev2_ptr)[0] = 0;
    (new_inode -> lev2_ptr)[1] = 0;
    new_inode -> lev3_ptr = 0;
    write_inode(new_inode -> ino);
    //adjust its father dir block
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    dentry_t *dentry = (inode_t*)(pa2kva(DATA_MEM_ADDR) + father_inode -> usize);
    dentry -> mode = FILE;
    dentry -> ino = new_inode -> ino;
    kstrcpy(dentry -> name, name);
    sbi_sd_write(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    //adjust its father dir inode
    father_inode -> nlinks ++;
    father_inode -> mtime = get_timer();
    father_inode -> usize += 32;
    write_inode(father_inode -> ino);
    //Finish create a file(like mkdir)
    printk("\nFinish create a file!\n");
}

fd_t file_d[MAXFD];
int do_fopen(char *name, u8 access){
    int i = 0;
    //Judge if there is a fs
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return -1;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    //find this file's inode
    //inode_t *father_inode = current_inode;
    //inode_t *find_file = lookup(father_inode, name, FILE);
    inode_t *father_inode = current_inode;
    int status = find_dir(name, FILE);
    inode_t *find_file;
    if(status == 1){
        find_file = current_inode;
    }else{
        printk("No such File\n");
        return;
    }
    current_inode = father_inode;
    if(find_file == NULL){
        printk(">Error: No such File\n");
        return -1;
    }
    //check access
    if(find_file -> access != RW_A && find_file -> access != access){
        printk(">PLV ERROR\n");
        return -1;
    }
    //alloc a fd
    for(i = 0; i < MAXFD; i++){
        if(file_d[i].ino == 0){
            break;
        }
    }
    int fd = i;
    file_d[i].access = access;
    file_d[i].r_ptr = 0;
    file_d[i].w_ptr = 0;
    file_d[i].ino = find_file->ino;
    //Return the index
    return fd;    
}

int do_fread(int fd, char *buff, int size){
    int i;
    char *temp_buff = buff;
    //Judge if there is a fs
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return -1;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    //get file inode
    inode_t *r_file = get_inode_from_ino(file_d[fd].ino);
    u32 r_ptr = file_d[fd].r_ptr;
    //printk("\nUsed size:", r_file -> usize);
    //Support big file
    //handle unalign(we just use lev1 ptr)
    int sz = size;
    u8 *r_buff;
    int pos = r_ptr - 40960;
    if(r_ptr % 512 != 0){
        //< direct ptr 
        if(r_ptr < 40960){
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + (r_file -> direct_ptr[r_ptr/4096])*8 + (r_ptr / 512) % 8);
            r_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            if(size > (512 - r_ptr % 512)){
                memcpy(temp_buff, &r_buff[r_ptr % 512], 512 - r_ptr % 512);
                sz -= (512 - r_ptr % 512);
                temp_buff += (512 - r_ptr % 512);
            }else{
                memcpy(temp_buff, &r_buff[r_ptr % 512], size);
                file_d[fd].r_ptr += size;
                sz -= size;
                temp_buff += size;
                return size;
            }
        }
        //< direct_ptr and lev 1 ptr(lev1_ptr -> 1K*4KB = 4MB), the larget is 4*3 = 12MB
        else{
            //read lev1 block into lev1 addr
            sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (r_file -> lev1_ptr[pos/(1 << 22)])*8);
            u32 *lev1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
            //read actual block according to lev1 block
            int pos1 = pos % (1 << 22);
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + lev1[pos1/4096]*8 + (pos1/512)%8);
            r_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            if(size > (512 - r_ptr % 512)){
                memcpy(temp_buff, &r_buff[r_ptr%512], 512 - r_ptr % 512);
                temp_buff += (512 - r_ptr % 512);
                sz -= (512 - r_ptr % 512);
            }else{
                memcpy(temp_buff, &r_buff[r_ptr % 512], 512 - r_ptr % 512);
                file_d[fd].r_ptr += size;
                sz -= size;
                temp_buff += size;
                return size;
            }
        }
    }
    //i: sector ID
    //If size < 40960
    int ac_size;
    for(i = (r_ptr + 511)/512; i < 80 && sz > 0; i++){
        //We read file every 512B
        sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + (r_file -> direct_ptr[i/8])*8 + i % 8);
        r_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
        ac_size = (sz > 512)? 512: sz;
        memcpy(temp_buff, r_buff, ac_size);
        temp_buff += ac_size;
        sz   -= ac_size;
    }
    //If size > 40960(32768=1024*8)
    int j;
    if(i >= 80 && sz > 0 && i < 8272){
        sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (r_file -> lev1_ptr[0])*8);
        u32 *lev1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
        for(; (i < 8192+80) && sz > 0; i++){
            j = i - 80;
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + lev1[j/8]*8 + j % 8);
            r_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            ac_size = (sz > 512)? 512:sz;
            memcpy(temp_buff, r_buff, ac_size);
            temp_buff += ac_size;
            sz   -= ac_size;
        }   
    }
    //if size > 40KB + 4MB(TO 40KB + 8MB)
    if(i >= 8272 && sz > 0){
        sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (r_file -> lev1_ptr[1])*8);
        u32 *lev1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
        for(; i < (8272+8192) && sz > 0; i++){
            j = i - 8272;
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + lev1[j/8]*8 + j % 8);
            r_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            ac_size = (sz > 512)? 512:sz;
            memcpy(temp_buff, r_buff, ac_size);
            temp_buff += ac_size;
            sz   -= ac_size;
        }
    }
    file_d[fd].r_ptr += size;
    return (size - sz);
}

int do_fwrite(int fd, char *buff, int size){
    int i;
    //Judge if there is a fs
    char *temp_buff = buff;
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return -1;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    //get file inode
    inode_t *w_file = get_inode_from_ino(file_d[fd].ino);
    u32 w_ptr = file_d[fd].w_ptr;
    //Support big file
    //handle unalign(we just use lev1 ptr)
    u8 *w_buff;
    int sz = size;
    int w_pos = w_ptr - 40960;
    if(w_ptr % 512 != 0){
        if(w_ptr < 40960){
            if(w_file -> direct_ptr[w_ptr / 4096] == 0){
                w_file -> direct_ptr[w_ptr / 4096] = alloc_data_block();
                write_inode(w_file -> ino);
                w_file = get_inode_from_ino(w_file -> ino);
            }
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + (w_file -> direct_ptr[w_ptr/4096])*8 + (w_ptr/512)%8);
            w_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            if(size > (512 - w_ptr % 512)){
                memcpy(&w_buff[w_ptr % 512], temp_buff, 512 - w_ptr % 512);
                sz -= 512 - w_ptr % 512;
                temp_buff += 512 - w_ptr % 512;
                sbi_sd_write(DATA_MEM_ADDR, 1, FS_START + (w_file -> direct_ptr[w_ptr/4096])*8 + (w_ptr/512)%8);
            }else{
                memcpy(&w_buff[w_ptr % 512], temp_buff, size);
                sz -= size;
                temp_buff += size;
                sbi_sd_write(DATA_MEM_ADDR, 1, FS_START + (w_file -> direct_ptr[w_ptr/4096])*8 + (w_ptr/512)%8);
                file_d[fd].w_ptr += size;
                w_file -> usize = file_d[fd].w_ptr;
                write_inode(w_file -> ino);
                return size;
            }   
        }else{
            if(w_file -> lev1_ptr[w_pos / (1 << 22)] == 0){
                w_file -> lev1_ptr[w_pos / (1 << 22)] = alloc_data_block();
                write_inode(w_file -> ino);
                w_file = get_inode_from_ino(w_file -> ino);
                int clear_buff[1024];
                for(int j = 0; j < 1024; j++){
                    clear_buff[j] = 0;
                }
                sbi_sd_write(kva2pa(clear_buff), 8, FS_START + (w_file -> lev1_ptr[w_pos / (1 << 22)])*8);
            }
            sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (w_file -> lev1_ptr[w_pos/(1 << 22)])*8);
            u32 *level1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
            int w_pos1 = w_pos % (1 << 22);
            if(level1[w_pos1/4096] == 0){
               level1[w_pos1/4096] = alloc_data_block();
                sbi_sd_write(kva2pa(level1), 8, FS_START + (w_file -> lev1_ptr[w_pos/(1 << 22)])*8);
                sbi_sd_read(kva2pa(level1), 8, FS_START + (w_file -> lev1_ptr[w_pos/(1 << 22)])*8);
            }
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + level1[w_pos1/4096]*8 + (w_pos1/512)%8);
            w_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            if(size > (512 - w_ptr % 512)){
                memcpy(&w_buff[w_ptr % 512], temp_buff, 512 - w_ptr % 512);
                sz -= 512 - w_ptr % 512;
                temp_buff += 512 - w_ptr % 512;
                sbi_sd_write(DATA_MEM_ADDR, 1, FS_START + level1[w_pos1/4096]*8 + (w_pos1/512)%8);
            }else{
                memcpy(&w_buff[w_ptr % 512], temp_buff, size);
                sz -= size;
                temp_buff += size;
                sbi_sd_write(DATA_MEM_ADDR, 1, FS_START + level1[w_pos1/4096]*8 + (w_pos1/512)%8);
                file_d[fd].w_ptr += size;
                w_file -> usize = file_d[fd].w_ptr;
                write_inode(w_file -> ino);
                return size;
            }
        }
    }
    int ac_size;
    for(i = (w_ptr + 511)/512; i < 80 && sz > 0; i++){
        //Does not alloc block
        if(w_file -> direct_ptr[i / 8] == 0){
            w_file -> direct_ptr[i / 8] = alloc_data_block();
            //w_file -> usize += 4096;
            write_inode(w_file -> ino);
            w_file = get_inode_from_ino(file_d[fd].ino);
        }
        sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + (w_file -> direct_ptr[i/8])*8 + i % 8);
        w_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
        ac_size = (sz > 512)? 512: sz;
        memcpy(w_buff, temp_buff, ac_size);
        sbi_sd_write(DATA_MEM_ADDR, 1, FS_START + (w_file -> direct_ptr[i/8])*8 + i % 8);
        sz -= ac_size;
        temp_buff += ac_size;
    }
    int j;
    if(i >= 80 && sz > 0 && i < 8272){
        if(w_file -> lev1_ptr[0] == NULL){
            w_file -> lev1_ptr[0] = alloc_data_block();
            //w_file -> usize += 4096;
            write_inode(w_file -> ino);
            w_file = get_inode_from_ino(file_d[fd].ino);
            int clear_buff[1024];
            for(j = 0; j < 1024; j++){
                clear_buff[j] = 0;
            }
            sbi_sd_write(kva2pa(clear_buff), 8, FS_START + (w_file -> lev1_ptr[0])*8);
        }
        sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (w_file -> lev1_ptr[0])*8);
        u32 *lev1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
        for(; i < (8192+80) && sz > 0; i++){
            j = i - 80;
            if(lev1[j / 8] == 0){
                lev1[j / 8] = alloc_data_block();
                //w_file -> usize += 4096;
                sbi_sd_write(kva2pa(lev1), 8, FS_START + (w_file -> lev1_ptr[0])*8);
                sbi_sd_read(kva2pa(lev1), 8, FS_START + (w_file -> lev1_ptr[0])*8);
            }
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + lev1[j/8]*8 + j % 8);   
            w_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            ac_size = (sz > 512)? 512: sz;
            memcpy(w_buff, temp_buff, ac_size);
            sbi_sd_write(DATA_MEM_ADDR, 1, FS_START + lev1[j/8]*8 + j % 8);
            sz -= ac_size;
            temp_buff += ac_size;
        }
    }
    if(i >= 8272 && sz > 0){
        if(w_file -> lev1_ptr[1] == NULL){
            w_file -> lev1_ptr[1] = alloc_data_block();
            //w_file -> usize += 4096;
            write_inode(w_file -> ino);
            w_file = get_inode_from_ino(file_d[fd].ino);
            int clear_buff[1024];
            for(j = 0; j < 1024; j++){
                clear_buff[j] = 0;
            }
            sbi_sd_write(kva2pa(clear_buff), 8, FS_START + (w_file -> lev1_ptr[1])*8);
        }
        sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (w_file -> lev1_ptr[1])*8);
        u32 *lev1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
        for(; i < (8192+8272) && sz > 0; i++){
            j = i - 8272;
            if(lev1[j / 8] == 0){
                lev1[j / 8] = alloc_data_block();
                sbi_sd_write(kva2pa(lev1), 8, FS_START + (w_file -> lev1_ptr[1])*8);
                sbi_sd_read(kva2pa(lev1), 8, FS_START + (w_file -> lev1_ptr[1])*8);
            }
            sbi_sd_read(DATA_MEM_ADDR, 1, FS_START + lev1[j/8]*8 + j % 8);   
            w_buff = (u8*)(pa2kva(DATA_MEM_ADDR));
            ac_size = (sz > 512)? 512: sz;
            memcpy(w_buff, temp_buff, ac_size);
            sbi_sd_write(DATA_MEM_ADDR, 1, FS_START + lev1[j/8]*8 + j % 8);
            sz -= ac_size;
            temp_buff += ac_size;
        }
    }
    file_d[fd].w_ptr += size;
    w_file -> usize = file_d[fd].w_ptr;
    write_inode(w_file -> ino);
    return size;
}

void do_fclose(int fd){
    file_d[fd].ino = 0;
    file_d[fd].access = 0;
    file_d[fd].r_ptr = file_d[fd].w_ptr = 0;
}

void do_cat(char *name){
    //Judge if there is a fs
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return -1;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    inode_t *father_inode = current_inode;
    int status = find_dir(name, FILE);
    inode_t *find_file;
    if(status == 1){
        find_file = current_inode;
    }else{
        printk("No such File\n");
        return;
    }
    current_inode = father_inode;
    if(find_file == NULL){
        printk("Error: Cannot find this file!\n");
        return;
    }
    char *buf;
    int cnt = 0;
    //printk("\n");
    //printk("File_Size: %d\n",find_file -> usize);
    //printk("ID: %d\n", find_file -> direct_ptr[0]);
    //printk("INO: %d\n", find_file -> ino);
   
    //sbi_sd_read(DATA_MEM_ADDR, 8, (find_file -> direct_ptr[0])*8 + FS_START);
    //buf = (char*)(pa2kva(DATA_MEM_ADDR));
    printk("\n");
    //< 40KB
    int i;
    for(i = 0; i < find_file -> usize && i < 40960; i += 4096){
        for(int j = 0; j < 8; j++){
            clears(pa2kva(DATA_MEM_ADDR) + j*512);
        }
        sbi_sd_read(DATA_MEM_ADDR, 8, (find_file -> direct_ptr[i/4096])*8 + FS_START);
        buf = (char*)(pa2kva(DATA_MEM_ADDR));
        for(int k = 0; k < 4096 && cnt < find_file -> usize; k++, cnt++){
            printk("%c",buf[k]);
        }
    }
    //int cnt = 0;
}

void do_lseek(int fd, int offset, int mode){
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return -1;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    inode_t *l_file = get_inode_from_ino(file_d[fd].ino);
    if(mode == SEEK_SET){
        file_d[fd].r_ptr = offset;
        file_d[fd].w_ptr = offset;
    }else if (mode == SEEK_CUR){
        file_d[fd].r_ptr += offset;
        file_d[fd].w_ptr += offset;
    }else if(mode == SEEK_END){
        file_d[fd].r_ptr = l_file -> usize + offset;
        file_d[fd].w_ptr = l_file -> usize + offset;
    }
}

//link
void do_ln(char *src, char *dst){
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return -1;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }

    inode_t *father_inode = current_inode;
    int found = find_dir(src, FILE);
    inode_t *find_file;
    if(found == 1){
        find_file = current_inode;
    }else{
        printk("No such file!\n");
        return;
    }
    current_inode = father_inode;
    //create a hard link
    //adjust its father dentry
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    dentry_t *dentry = (dentry_t*)(pa2kva(DATA_MEM_ADDR) + father_inode -> usize);
    dentry -> mode = FILE;
    dentry -> ino = find_file -> ino;
    kstrcpy(dentry -> name, dst);
    sbi_sd_write(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    //adjust dir inode
    father_inode -> nlinks ++;
    father_inode -> usize += 32;
    father_inode -> mtime = get_timer();
    write_inode(father_inode->ino);

    find_file -> nlinks ++;
    write_inode(find_file -> ino);
    printk("\nFinish create a hard link\n");
}

void do_rm(char *name){
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("Error: No file System!\n");
        return -1;
    } 
    //check current_node, if not ---> root dir
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }

    inode_t *father_inode = current_inode;
    //printk("\nName: %s\n", name);
    inode_t *find_file = lookup(father_inode, name, FILE);
    //printk("\nfind_file -> ino: %d\n", find_file -> ino);
    //dentry
    if(find_file == NULL){
        printk("Error: No Such File\n");
        return;
    }
    int found1 = 0;
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    dentry_t *dentry = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    for(int i = 0; i < father_inode -> usize; i += 32){
        if(found1){
            memcpy((u8*)(dentry - 1), (u8*)dentry, 32);
        }
        if(!strcmp(name, dentry->name)){
            //printk("dentry -> name", dentry -> name);
            found1 = 1;
        }
        dentry ++;
    }
    sbi_sd_write(DATA_MEM_ADDR, 8, FS_START + (father_inode -> direct_ptr[0])*8);
    //inode
    father_inode -> usize -= 32;
    father_inode -> nlinks --;
    father_inode -> mtime = get_timer();
    write_inode(father_inode -> ino);

    find_file -> nlinks --;
    int j = 0;
    if(find_file -> nlinks == 0){
        clear_inode_map(find_file -> ino);
        for(int j = 0; j < 10; j++){
            if(find_file -> direct_ptr[j] != 0){
                clear_data_block(find_file -> direct_ptr[j]);
            }
        }
        if(find_file -> usize > 40960){
           // printk("\nBegin clear lev1\n");
            sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (find_file -> lev1_ptr[0])*8);
            u32 *lev1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
            for(int k = 0; k < 1024; k++){
                if(lev1[k] != 0){
                    clear_data_block(lev1[k]);
                }
            }
            clear_data_block(find_file -> lev1_ptr[0]);
        }
        if(find_file -> usize > ((1 << 22) + 40960) && find_file -> usize < ((1 << 23) + 40960)){
            printk("\nBegin clear lev1\n");
            sbi_sd_read(LEV1_MEM_ADDR, 8, FS_START + (find_file -> lev1_ptr[1])*8);
            u32 *lev1 = (u32*)(pa2kva(LEV1_MEM_ADDR));
            for(int k = 0; k < 1024; k++){
                if(lev1[k] != 0){
                    clear_data_block(lev1[k]);
                }
            }
            clear_data_block(find_file -> lev1_ptr[1]);
        }
    }else{
        write_inode(find_file -> ino);
    }
    printk("\nFinish delete\n");
}

void do_ls_l(){
    sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
    superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
    if(sp -> magic != MAGIC){
        printk("fs does not exit!\n");
        return;
    }
    sbi_sd_read(INODE_MEM_ADDR, 1, FS_START + INODE_SD_OFFSET);
    if(state == 0){
        current_inode = (inode_t*)(pa2kva(INODE_MEM_ADDR));
        state = 1;
    }
    sbi_sd_read(DATA_MEM_ADDR, 8, FS_START + (current_inode -> direct_ptr[0])*8);
    dentry_t *d = (dentry_t*)(pa2kva(DATA_MEM_ADDR));
    printk("\n");
    for(int i = 0; i < current_inode -> usize; i+=32){
        inode_t *file = get_inode_from_ino(d[i/32].ino);
        printk("%s used_size: %dB nlinks: %d ino %d\n", d[i/32].name, file -> usize, file -> nlinks, file -> ino);
    }
    printk("\n");
}