#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>
#include <os/string.h>
//#include <mailbox.h>
#include <stdio.h>
#define MAXLOCK 100
#define MAXSEM  100
#define MAXMBOX 15

//mutex_lock_t lock_arr[MAXLOCK];
mthread_semaphore sem_arr[MAXSEM];
int lock_dex = 0;

static inline void assert_supervisor_mode() 
{ 
   __asm__ __volatile__("csrr x0, sscratch\n"); 
} 


/*void init_lock(void){
    lock_arr = kmalloc(sizeof(*lock_arr)*MAXLOCK);
    for(int i = 0; i < MAXLOCK; i++){
      lock_arr[i] = kmalloc(sizeof(lock_arr));
    }
}*/


//Task 2's code
void do_mutex_lock_init(int* lock)
{
    /* TODO */
    //init_lock();
    /*mutex_lock_t *p = &lock_arr[(int)lock % 50];
    (p -> block_queue) = (list_head*)kmalloc(sizeof(list_head));
    init_list_head((p -> block_queue));     //init block_queue
    p -> lock.status = UNLOCKED;             //init the status
    *lock = (int)lock % 100;
    lock_dex++;*/
    do_sem_init(lock, 1);
}

void do_sem_down_lock(int *sem){
    mthread_semaphore *p = &sem_arr[*sem];
    long cpu_id = get_current_cpu_id();
    current_running[cpu_id] -> lock[current_running[cpu_id] -> lock_num] = p;
    current_running[cpu_id] -> lock_num ++;
    if(p -> s > 0){
        (p -> s) --;
    }else{
        current_running[cpu_id] -> status = TASK_BLOCKED;
        list_add_tail(&(current_running[cpu_id] -> list), &(p -> wait_queue));
        do_scheduler();
    }
}

void do_mutex_lock_acquire(int* lock)
{
    /* TODO */
    //First we should judge if the lock is acquired
    /*mutex_lock_t *p = &lock_arr[*lock];
    if(p -> lock.status == LOCKED){
        //If this lock has already been acuquired
        do_block(&(current_running -> list), (p -> block_queue));
    }else{
        //Else: set the lock status: Locked
        p -> lock.status = LOCKED;
        //These codes aim to add locks to our PCB 
        current_running -> lock[current_running -> lock_num] = p;
        current_running -> lock_num ++;
    }*/
    mthread_semaphore *p = &sem_arr[*lock];
    /*if(p -> s > 0){
        long cpu_id = get_current_cpu_id();
        current_running[cpu_id] -> lock[current_running[cpu_id] -> lock_num] = p;
        current_running[cpu_id] -> lock_num ++;
    }*/
    do_sem_down_lock(lock);
}

void do_mutex_lock_release(int* lock)
{
    /* TODO */
    /*mutex_lock_t *p = &lock_arr[*lock];
    //We need to release this LOCK in the current_running's PCB!
    for(int i = 0; i < current_running -> lock_num; i++){
        if(current_running -> lock[i] == p){
            for(int k = i + 1; k < current_running -> lock_num; k++){
                current_running -> lock[k-1] = current_running -> lock[k];      //We move our lock FORWARD and EXEC
            }
            --current_running -> lock_num;
            break;      //If we find this lock, we jump out the loop.
        }
    }

    if(!list_empty((p -> block_queue))){
        //Only when block_queue is not empty, we need to release
        do_unblock((p -> block_queue));
        //If the queue is not empty, the status is locked
    }else{
        //If the queue is empty. The status is unlocked
        p -> lock.status = UNLOCKED;
    }*/
    long cpu_id = get_current_cpu_id();
    do_sem_up(lock);
    current_running[cpu_id] -> lock_num --;
}


/*----------------------------------P3 ADD CODE------------------------------------------------*/



void do_sem_init(int *sem, int val){
    mthread_semaphore *p;
    int i = 0;
    while((&sem_arr[(int)sem % 150 + i]) -> status == 1){
        i++;
    }
    p = &sem_arr[(int)sem % 150 + i];
    init_list_head(&(p -> wait_queue));
    *sem = (int)sem % 150+ i;
    p -> s = val;
    p -> status = 1;
    i = 0;
}

void do_sem_up(int *sem){
    mthread_semaphore *p = &sem_arr[*sem];

    if(!list_empty(&(p -> wait_queue))){
        pcb_t *she = list_entry((&(p -> wait_queue)) -> next, pcb_t, list);
        she -> status = TASK_READY;
        list_del((&(p -> wait_queue)) -> next);
        list_add_tail(&(she -> list), &ready_queue);
    }else{
        (p -> s) ++;
    }
}


void do_sem_down(int *sem){
    mthread_semaphore *p = &sem_arr[*sem];
     long cpu_id = get_current_cpu_id();
    
    if(p -> s > 0){
        (p -> s) --;
    }else{
        current_running[cpu_id] -> status = TASK_BLOCKED;
        list_add_tail(&(current_running[cpu_id] -> list), &(p -> wait_queue));
        do_scheduler();
    }
}



void do_sem_destroy(int *sem){
    mthread_semaphore *p = &sem_arr[*sem];

    while(!list_empty(&(p -> wait_queue))){
        pcb_t *she = list_entry((&(p -> wait_queue)) -> next, pcb_t, list);
        she -> status = TASK_READY;
        list_del((&(p -> wait_queue)) -> next);
        list_add_tail(&(she -> list), &ready_queue);
    }
    p -> status = 0;

}

//Barrier

void do_barrier_init(int *barrier, unsigned count){
    mthread_semaphore *p = &sem_arr[(int)barrier % 150];
    init_list_head(&(p -> wait_queue));
    *barrier = (int)barrier % 150;
    p -> s = (int)count;
    p -> status = 1;
}

void do_barrier_wait(int *barrier){
    mthread_semaphore *p = &sem_arr[*barrier];
    long cpu_id = get_current_cpu_id();
    if(p -> s > 1){
        p -> s --;
        list_add_tail(&(current_running[cpu_id] -> list), &(p -> wait_queue));
        current_running[cpu_id] -> status = TASK_BLOCKED;
        do_scheduler();
    }else{
        while(!list_empty(&(p -> wait_queue))){
            pcb_t *she = list_entry((&(p -> wait_queue)) -> next, pcb_t, list);
            she -> status = TASK_READY;
            list_del((&(p -> wait_queue)) -> next);
            list_add_tail(&(she -> list), &ready_queue);
            p -> s ++;
        }
        
    }
}

void do_barrier_detroy(int *barrier){
    do_sem_destroy(barrier);
}

//Mbox
mailbox_k mbox_arr[MAXMBOX];

int do_mbox_open(char *name){
    int i = 0;
    //If find, we return the mbox's id
    for(i = 0; i < MAXMBOX; i++){
        if(strcmp(name, mbox_arr[i].name) == 0){
            return i;
        }
    }
    //If not find, we create a new mbox
    for(i = 0; i < MAXMBOX; i++){
        if(mbox_arr[i].status != 1){        //Not Busy
            int j = 0;
            mbox_arr[i].status = 1;
            while(name[j] != '\0'){
                mbox_arr[i].name[j] = name[j];
                j++;
            }
            mbox_arr[i].name[j] = '\0';
            mbox_arr[i].index = 0;
            mbox_arr[i].empty.wait_queue = (list_head*)kmalloc(sizeof(list_head));
            init_list_head(mbox_arr[i].empty.wait_queue);
            mbox_arr[i].full.wait_queue  = (list_head*)kmalloc(sizeof(list_head));
            init_list_head(mbox_arr[i].full.wait_queue);
            mbox_arr[i].empty.block_count = 0;
            mbox_arr[i].full.block_count = 0;
            return i;
        }
    }
    return -1;      //No available Mbox
}

void do_mbox_close(int mailbox){
    mbox_arr[mailbox].status = 0;       //Close Status
    mbox_arr[mailbox].index = 0;
    mbox_arr[mailbox].empty.block_count = 0;
    mbox_arr[mailbox].full.block_count = 0;
    int i = 0;
    while(mbox_arr[mailbox].name[i] != '\0'){
        mbox_arr[mailbox].name[i] = 0;
        i++;
    }//Clear the name
}

int do_mbox_send(int mailbox, void *msg, int msg_length){

    while(mbox_arr[mailbox].index + msg_length > 64){
        long cpu_id = get_current_cpu_id();
        current_running[cpu_id] -> status = TASK_BLOCKED;
        list_add_tail(&(current_running[cpu_id] -> list), mbox_arr[mailbox].full.wait_queue);
        mbox_arr[mailbox].full.block_count ++;
        do_scheduler();
    }
    if(mbox_arr[mailbox].index + msg_length <= 64){
        int i = 0;
        for(i = 0; i < msg_length; i++){
            mbox_arr[mailbox].msg[mbox_arr[mailbox].index++] = ((char*)msg)[i];
        }
        while(!list_empty(mbox_arr[mailbox].empty.wait_queue)){
            pcb_t *she;
            she = list_entry(mbox_arr[mailbox].empty.wait_queue -> next, pcb_t, list);
            she -> status = TASK_READY;
            list_del(mbox_arr[mailbox].empty.wait_queue -> next);
            list_add_tail(&(she -> list), &ready_queue);
            //do_scheduler();
        }
        //do_scheduler();
    }
    return mbox_arr[mailbox].full.block_count; 
}


int do_mbox_recv(int mailbox, void *msg, int msg_length){
    while(mbox_arr[mailbox].index - msg_length < 0){
        long cpu_id = get_current_cpu_id();
        current_running[cpu_id] -> status = TASK_BLOCKED;
        list_add_tail(&(current_running[cpu_id] -> list), mbox_arr[mailbox].empty.wait_queue);
        mbox_arr[mailbox].empty.block_count ++;
        do_scheduler();
    }
    if(mbox_arr[mailbox].index - msg_length >= 0){
        int i = 0;
        for(i = 0; i < msg_length; i++){
            ((char*)msg)[i] = mbox_arr[mailbox].msg[i];
        }
        //Remove Data 
        for(i = msg_length; i < mbox_arr[mailbox].index; i++){
            mbox_arr[mailbox].msg[i - msg_length] = mbox_arr[mailbox].msg[i];
        }
        mbox_arr[mailbox].index -= msg_length;

        while(!list_empty(mbox_arr[mailbox].full.wait_queue)){
            pcb_t *she;
            she = list_entry(mbox_arr[mailbox].full.wait_queue -> next, pcb_t, list);
            she -> status = TASK_READY;
            list_del(mbox_arr[mailbox].full.wait_queue -> next);
            list_add_tail(&(she -> list), &ready_queue);
            //do_scheduler();
        }
    }
    return mbox_arr[mailbox].empty.block_count;
}

//Prj4 lock
int do_binsem_get(int key){
    mthread_semaphore *p;
    int i = 0;
    /*while((&sem_arr[key % 150 + i]) -> status == 1){
        i++;
    }*/
    if((&sem_arr[key % 150 + i]) -> status == 1){
        return key%150;
    }
    p = &sem_arr[key % 150 + i];
    int id = key % 150 + i;
    init_list_head(&(p -> wait_queue));
    p -> s = 1;
    p -> status = 1;
    i = 0;
    return id;
}

int do_binsem_op(int binsem_id, int op){
    if(op == 0){
        do_mutex_lock_acquire(&binsem_id);
        return 1;
    }else if(op == 1){
        do_mutex_lock_release(&binsem_id);
        return 1;
    }
    return 0;
}

