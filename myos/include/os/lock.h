/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                                   Thread Lock
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

#ifndef INCLUDE_LOCK_H_
#define INCLUDE_LOCK_H_

#include <os/list.h>
//#include <mailbox.h>

typedef enum {
    UNLOCKED,
    LOCKED,
} lock_status_t;

typedef struct spin_lock
{
    volatile lock_status_t status;
} spin_lock_t;
 
typedef struct mutex_lock
{
    spin_lock_t lock;
    list_head *block_queue;
    int status;
} mutex_lock_t;

/* init lock */
void spin_lock_init(spin_lock_t *lock);
int spin_lock_try_acquire(spin_lock_t *lock);
void spin_lock_acquire(spin_lock_t *lock);
void spin_lock_release(spin_lock_t *lock);

void do_mutex_lock_init(int *lock);
void do_mutex_lock_acquire(int *lock);
void do_mutex_lock_release(int *lock);

//Sem Operation
void do_sem_init(int *sem, int val);
void do_sem_up(int *sem);
void do_sem_down(int *sem);
void do_sem_destroy(int *sem);

//Barrier Operation
void do_barrier_init(int *barrier, unsigned count);
void do_barrier_wait(int *barrier);
void do_barrier_detroy(int *barrier);

//Mbox Operation
int do_mbox_open(char *name);
void do_mbox_close(int mailbox);
int do_mbox_send(int mailbox, void *msg, int msg_length);
int do_mbox_recv(int mailbox, void *msg, int msg_length);

//Binsem
int do_binsem_get(int key);
int do_binsem_op(int binsem_id, int op);

//Sem structure
typedef struct mthread_semaphore1
{
    // TODO:
    int s;
    list_node_t wait_queue;
    int status;
} mthread_semaphore;

typedef struct mthread_semaphore2
{
    // TODO:
    int s;
    list_node_t* wait_queue;
    int block_count;
} mthread_semaphore_k;

//Mailbox Struture
typedef struct mailbox_k_t{
    int status;
    char name[25];
    char msg[64];
    int index;
    //int block_count;
    mthread_semaphore_k full;
    mthread_semaphore_k empty;
}mailbox_k;

#endif
