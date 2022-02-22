/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *        Process scheduling related content, such as: scheduler, process blocking,
 *                 process wakeup, process creation, process kill, etc.
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

#ifndef INCLUDE_SCHEDULER_H_
#define INCLUDE_SCHEDULER_H_

#include <type.h>
#include <os/list.h>
#include <os/mm.h>
#include <os/time.h>
#include <os/lock.h>
#include <context.h>

#define NUM_MAX_TASK 50

#define MAX_LOCK 20

/* used to save register infomation */
/*typedef struct regs_context
{
    
    reg_t regs[32];

    
    reg_t sstatus;
    reg_t sepc;
    reg_t sbadaddr;
    reg_t scause;
} regs_context_t;


typedef struct switchto_context
{
    
    reg_t regs[14];
} switchto_context_t;*/

typedef enum {
    TASK_BLOCKED,
    TASK_RUNNING,
    TASK_READY,
    TASK_ZOMBIE,
    TASK_EXITED,
} task_status_t;

typedef enum {
    ENTER_ZOMBIE_ON_EXIT,
    AUTO_CLEANUP_ON_EXIT,
} spawn_mode_t;


typedef enum {
    KERNEL_PROCESS,
    KERNEL_THREAD,
    USER_PROCESS,
    USER_THREAD,
} task_type_t;

/* Process Control Block */
typedef struct pcb
{
    /* register context */
    // this must be this order!! The order is defined in regs.h
    reg_t kernel_sp;
    reg_t user_sp;

    // count the number of disable_preempt
    // enable_preempt enables CSR_SIE only when preempt_count == 0
    reg_t preempt_count;

    ptr_t kernel_stack_base;
    ptr_t user_stack_base;

    /* previous, next pointer */
    list_node_t list;
    list_head wait_list;

    /* process id */
    pid_t pid;

    /*thread id*/
    pid_t tid;
    /* kernel/user thread/process */
    task_type_t type;

    /* BLOCK | READY | RUNNING */
    task_status_t status;
    spawn_mode_t mode;
 
    /* cursor position */
    int cursor_x;
    int cursor_y;

    /*sleep*/
    timer timer1;    //when this process is awake

    /*Priority count*/
    int pri;
    
    int tick;


    /*Lock: We need to bound our lock with PCB to release Lock when killed or exited*/
    int lock_num;
    mthread_semaphore *lock[MAX_LOCK];

    /*PGDIR addr*/
    uint64_t* pgdir;

    list_head plist;
    int pnum;

    /*To mem tid*/
    pid_t tid_mem;

} pcb_t;

/* task information, used to init PCB */
typedef struct task_info
{
    ptr_t entry_point;
    task_type_t type;
    int pri;
} task_info_t;

/* ready queue to run */
extern list_head ready_queue;

/* block quene waiting*/
extern list_head block_queue;

/* current running task PCB */
extern pcb_t * volatile current_running[2];
extern pid_t process_id;

extern pcb_t pcb[NUM_MAX_TASK];
extern pcb_t pid0_pcb_0;      
extern pcb_t pid0_pcb_1;      //The initial PID
extern const ptr_t pid0_stack_0;
extern const ptr_t pid0_stack_1;

extern void switch_to(pcb_t *prev, pcb_t *next);
void do_scheduler(void);
void do_sleep(uint32_t);


void do_block(list_node_t *, list_head *queue);
void do_unblock(list_node_t *);

extern pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode);
extern void do_exit(void);
extern int do_kill(pid_t pid, pid_t tid);
extern int do_waitpid(pid_t pid);
extern void do_process_show();
extern pid_t do_getpid();

extern pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode);
extern void do_show_exec();
extern int do_mthread_create(pid_t *thread, void (*start_routine)(void*), char *argv[]);

#endif
