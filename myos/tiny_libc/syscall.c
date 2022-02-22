#include <sys/syscall.h>
#include <stdint.h>

extern void vt100_move_cursor(int x, int y);
extern void do_scheduler();

void sys_sleep(uint32_t time)
{
    // TODO:
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    // TODO:
    invoke_syscall(SYSCALL_WRITE, (uintptr_t)buff, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    // TODO:
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    // TODO:
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE);
}

long sys_get_timebase()
{
    // TODO:
    long time = invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
    return time;
}

long sys_get_tick()
{
    // TODO:
   long time = invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
   return time;
}

void sys_yield()
{
    // TODO:
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
    //   or
    // do_scheduler();
    // ???
}


/*--------------------------------------P3 ADD SYSCALL----------------------------------------------*/

int sys_get_char(){
    int c = (int)invoke_syscall(SYSCALL_GET_CHAR, IGNORE, IGNORE, IGNORE, IGNORE);
    return c;    
}

void sys_put_char(char ch){
    invoke_syscall(SYSCALL_PUT_CHAR, ch, IGNORE, IGNORE, IGNORE);
}

void sys_process_show(void){
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void){
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_exit(void){
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_kill(pid_t pid, pid_t tid){
    int kill_status = (int)invoke_syscall(SYSCALL_KILL, pid, tid, IGNORE, IGNORE);
    return kill_status;
}

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode){
    pid_t pid = (pid_t)invoke_syscall(SYSCALL_SPAWN, (uintptr_t)info, (uintptr_t)arg, mode, IGNORE);
    return pid;
}

int sys_waitpid(pid_t pid){
    int wait_status = (int)invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
    return wait_status;
}

pid_t sys_getpid(){
    pid_t pid1 = (pid_t)invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
    return pid1;
}

int binsemget(int key){
    int id;
    id = invoke_syscall(SYSCALL_BINSEMGET, key, IGNORE, IGNORE, IGNORE);
    return id;
}

int binsemop(int binsem_id, int op){
    int status;
    status = invoke_syscall(SYSCALL_BINSEMOP, binsem_id, op, IGNORE, IGNORE);
    return status;
}

pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode){
    pid_t pid = invoke_syscall(SYSCALL_EXEC, file_name, argc, argv, mode);
    return pid;
}

void sys_show_exec(){
    invoke_syscall(SYSCALL_SHOW_EXEC, IGNORE, IGNORE, IGNORE, IGNORE);
}

/*P5 Network Syscall*/
void sys_net_send(uintptr_t addr, size_t length){
    invoke_syscall(SYSCALL_NET_SEND, addr, length, IGNORE, IGNORE);
}

long sys_net_recv(uintptr_t addr, size_t length, int num_packet, int* frLength){
    long ret;
    ret = invoke_syscall(SYSCALL_NET_RECV, addr, frLength, num_packet, length);
    return ret;
}

void sys_net_irq_mode(int mode){
    invoke_syscall(SYSCALL_NET_IRQ_MODE, mode, IGNORE, IGNORE, IGNORE);
}

/*p6  FS*/
void sys_mkfs(){
    invoke_syscall(SYSCALL_MKFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_statfs(){
    invoke_syscall(SYSCALL_STATFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_mkdir(char *file_name){
    invoke_syscall(SYSCALL_MKDIR, file_name, IGNORE,IGNORE, IGNORE);
}

void sys_cd(char *path){
    invoke_syscall(SYSCALL_CD, path, IGNORE, IGNORE, IGNORE);
}

void sys_ls(char *path){
    invoke_syscall(SYSCALL_LS, path, IGNORE, IGNORE, IGNORE);
}

void sys_rmdir(char *file_name){
    invoke_syscall(SYSCALL_RMDIR, file_name, IGNORE, IGNORE, IGNORE);
}

void sys_touch(char *file_name){
    invoke_syscall(SYSCALL_TOUCH, file_name, IGNORE, IGNORE, IGNORE);
}

int sys_fopen(char *name, u8 mode){
    int fd;
    fd = invoke_syscall(SYSCALL_FOPEN, name, mode, IGNORE, IGNORE);
    return fd;
}

int sys_fread(int fd, char *buff, int size){
    int size1;
    size1 = invoke_syscall(SYSCALL_FREAD, fd, buff, size, IGNORE);
    return size1;
}

void sys_close(int fd){
    invoke_syscall(SYSCALL_FCLOSE, fd, IGNORE, IGNORE, IGNORE);
}

int sys_fwrite(int fd, char *buff, int size){
    int size1;
    size1 = invoke_syscall(SYSCALL_FWRITE, fd, buff, size, IGNORE);
    return size1;
}

void sys_cat(char *name){
    invoke_syscall(SYSCALL_CAT, name, IGNORE, IGNORE, IGNORE);
}

void sys_lseek(int fd, int offset, int whence){
    invoke_syscall(SYSCALL_LSEEK, fd, offset, whence, IGNORE);
}

void sys_ln(char *src, char *dst){
    invoke_syscall(SYSCALL_LN, src, dst, IGNORE, IGNORE);
}
 
void sys_rm(char *name){
    invoke_syscall(SYSCALL_RM, name, IGNORE, IGNORE, IGNORE);
}

void sys_ls_l(){
    invoke_syscall(SYSCALL_LS_L, IGNORE, IGNORE, IGNORE, IGNORE);
}