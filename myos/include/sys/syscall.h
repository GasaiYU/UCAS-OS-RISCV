/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                       System call related processing
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#ifndef INCLUDE_SYSCALL_H_
#define INCLUDE_SYSCALL_H_

#include <os/syscall_number.h>
#include <stdint.h>
#include <os.h>
#include <os/sched.h>

#define SCREEN_HEIGHT 80

extern long invoke_syscall(long, long, long, long, long);

pid_t sys_spawn(task_info_t *info, void* arg, spawn_mode_t mode);
pid_t sys_create_thread(uintptr_t entry_point, void* arg);
void sys_exit(void);
void sys_sleep(uint32_t time);
int sys_kill(pid_t pid, pid_t tid);
int sys_waitpid(pid_t pid);
pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode);
void sys_show_exec();

void sys_futex_wait(volatile uint64_t *val_addr, uint64_t val);
void sys_futex_wakeup(volatile uint64_t *val_addr, int num_wakeup);
int sys_mbox_open(char *name);
void sys_mbox_close(int mbox_id);
void sys_mbox_send(int mbox_id, void *msg, int msg_length);
void sys_mbox_recv(int mbox_id, void *msg, int msg_length);

void sys_write(char *);
void sys_move_cursor(int, int);
void sys_reflush();

long sys_get_timebase();
long sys_get_tick();

void sys_process_show(void);
void sys_screen_clear(void);
pid_t sys_getpid();
int sys_get_char();

#define BINSEM_OP_LOCK 0 // mutex acquire
#define BINSEM_OP_UNLOCK 1 // mutex release

int binsemget(int key);
int binsemop(int binsem_id, int op);

long sys_net_recv(uintptr_t addr, size_t length, int num_packet, int* frLength);
void sys_net_send(uintptr_t addr, size_t length);
void sys_net_irq_mode(int mode);

/*fs*/
void sys_mkfs();
void sys_statfs();
void sys_mkdir(char *file_name);
void sys_rmdir(char *file_name);
void sys_cd(char *path);
void sys_ls(char *path);
void sys_touch(char *file_name);

int sys_fopen(char *name, u8 mode);
int sys_fread(int fd, char *buff, int size);
void sys_close(int fd);
int sys_fwrite(int fd, char *buff, int size);
void sys_cat(char *name);
void sys_lseek(int fd, int offset, int whence);
void sys_ln(char *src, char *dst);
void sys_rm(char *name);
void sys_ls_l();
#endif
