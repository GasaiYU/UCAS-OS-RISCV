#include <os/list.h>
#include <os/mm.h>
#include <os/lock.h>
#include <os/sched.h>
#include <os/time.h>
#include <os/irq.h>
#include <screen.h>
#include <stdio.h>
#include <assert.h>
#include <os/stdio.h>
#include <pgtable.h>
#include <../include/os/elf.h>
#include <../include/user_programs.h>

pid_t tid[50];
pcb_t tcb[50];


extern void init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, void *arg, char *argv[],
    pcb_t *pcb);

extern void init_tcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, void *arg, char *argv[], uintptr_t user, 
    pcb_t *pcb);

pcb_t pcb[NUM_MAX_TASK];
//Master Core
const ptr_t pid0_stack_0 = INIT_KERNEL_STACK + PAGE_SIZE;
pcb_t pid0_pcb_0 = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_0,
    .user_sp = (ptr_t)pid0_stack_0,
    .preempt_count = 0,
    .pgdir = 0x5e000000
    //.status = TASK_RUNNING
};
//Slave Core
const ptr_t pid0_stack_1 = INIT_KERNEL_STACK_S + PAGE_SIZE;
pcb_t pid0_pcb_1 = {
    .pid = 0,
    .kernel_sp = (ptr_t)pid0_stack_1,
    .user_sp = (ptr_t)pid0_stack_1,
    .preempt_count = 0,
    .pgdir = 0x5e000000
    //.status = TASK_RUNNING
};


LIST_HEAD(ready_queue);

LIST_HEAD(exit_queue);

LIST_HEAD(timers);

/* current running task PCB */
pcb_t * volatile current_running[2];

/* global process id */
pid_t process_id = 2;       //Because Pid = 1 is the Shell Process 


//This function aim to get the next_running if preemintive shedule
pcb_t *find_next(list_node_t *head){
    pcb_t *find_pcb;        //The pcb which we aim to found
    pcb_t *now_pcb;         
    list_node_t *current_node = head -> next;
    int prio = -10;
    while(!list_empty(&ready_queue) && current_node != head){
        now_pcb = list_entry(current_node, pcb_t, list);
        if(now_pcb -> pri - now_pcb -> tick > prio ){
            prio = now_pcb -> pri - now_pcb -> tick;
            find_pcb = now_pcb;
        }
        current_node = current_node -> next;
    }
    /*if(find_pcb -> pri > 1){
        find_pcb ->pri --;
    }*/
    find_pcb -> tick++;
    return find_pcb;
}



void do_scheduler(void)
{
    // TODO schedule
    long cpu_id = get_current_cpu_id();
    // Modify the current_running pointer.
    pcb_t *next_running;     //Task which is running before switching
    pcb_t *p = current_running[cpu_id];
    //timer_check();  //check the sleeping queue
    
    
    if(current_running[cpu_id] -> status != TASK_BLOCKED && current_running[cpu_id] -> status != TASK_EXITED && current_running[cpu_id] -> status != TASK_ZOMBIE){
        current_running[cpu_id] -> status = TASK_READY;     //if blocked, its status will continue
        if(current_running[cpu_id] -> pid != 0){
            list_add_tail(&(current_running[cpu_id] -> list), &ready_queue);     //if not pid0, it will be added to ready queue
        }
    }

    if(!list_empty(&ready_queue)){
        //If preemitive, this should be changed
        next_running = list_entry(ready_queue.next, pcb_t, list);//find_next(&ready_queue);      //get next_running PCB
        list_del(&(next_running -> list));                                    //Delete this PCB in queue
        
    }else{
        if(cpu_id==0){
            next_running = &pid0_pcb_0;
        }else{
            next_running = &pid0_pcb_1;
        }
    }
        
    next_running -> status = TASK_RUNNING;
    current_running[cpu_id] = next_running;
    
    /*if(current_running -> pri - current_running -> tick < -5){
      current_running -> tick = 0;
    }*/
    // restore the current_runnint's cursor_x and cursor_y
    vt100_move_cursor(current_running[cpu_id]->cursor_x,
                      current_running[cpu_id]->cursor_y);
    
    //screen_cursor_x = current_running[cpu_id]->cursor_x;
    //screen_cursor_y = current_running[cpu_id]->cursor_y;
    //init_screen();
    //screen_reflush();
    // TODO: switch_to current_running
    set_satp(SATP_MODE_SV39, 1, (uintptr_t)(next_running -> pgdir) >> 12);
    local_flush_tlb_all();
    switch_to(p, next_running);
}

void do_sleep(uint32_t sleep_time)
{   
    long cpu_id = get_current_cpu_id();
    // TODO: sleep(seconds)
    // note: you can assume: 1 second = `timebase` ticks
    // 1. block the current_running
    current_running[cpu_id] -> status = TASK_BLOCKED;
    // 2. create a timer which calls `do_unblock` when timeout                                    
    current_running[cpu_id] -> timer1.time_finish = get_ticks() + sleep_time*get_time_base();
    list_del(&(current_running[cpu_id] -> list));
    list_add_tail(&(current_running[cpu_id] -> list), &timers);                                  
    // 3. reschedule because the current_running is blocked.
    do_scheduler();
}

void timer_check(void)
{
    list_node_t *current_node, *next_node;
    pcb_t *p;
    current_node = timers.next;
    while(!list_empty(&timers) && current_node != &timers){
        p = list_entry(current_node, pcb_t, list);        //get the pcb
        next_node = current_node -> next;                 //get the next node
        if(p -> timer1.time_finish <= get_ticks()){
            p -> status = TASK_READY;
            list_del(&(p -> list));
            list_add_tail(&(p -> list), &ready_queue);
        }
        current_node = next_node;                       //visit all the nodes in the queue
    }
}

void do_block(list_node_t *pcb_node, list_head *queue)
{
    // TODO: block the pcb task into the block queue
    list_add_tail(pcb_node,queue);   
    long cpu_id = get_current_cpu_id();           //Set the pcb_node into the block queue
    current_running[cpu_id] -> status = TASK_BLOCKED;   //Set the status: TAKS_BLOCKED
    do_scheduler();                             //do sheduler. Start the task.
}

void do_unblock(list_node_t *pcb_node)
{
    // TODO: unblock the `pcb` from the block queue
    //Move the pcb_node into the ready_queue and delete it in the Blocked_queue
    pcb_t *p = list_entry(pcb_node->next,pcb_t,list);
    p -> status = TASK_READY;
    list_del(pcb_node -> next);
    list_add_tail(&(p -> list), &ready_queue);
}

/*--------------------------------P3 ADD SHELL PRIMITIVE----------------------------------------------*/
//Global Process Pointer:(Because we have already initialized shell process)
int process_num = 1;

//Show Process Info(Shell Process also included) ---> sycall_ps()
void do_process_show(){
    prints("[PROCESS TABLE]\n");
    int j = 0;
    for(int i = 0; i < process_num; i++){
        for(int k = 0; k < tid[pcb[i].pid]; k++){
            if(k == 0){
                if(pcb[i].status == TASK_RUNNING){
                    prints("[%d] PID : %d  TID : %d STATUS : RUNNING\n",j ,pcb[i].pid, pcb[i].tid);
                    j++;
                }else if(pcb[i].status == TASK_READY){
                    prints("[%d] PID : %d  TID : %d STATUS : READY\n", j ,pcb[i].pid ,pcb[i].tid);
                    j++;
                }else if(pcb[i].status == TASK_BLOCKED){
                    prints("[%d] PID : %d  TID : %d STATUS : BLOCKED\n",j , pcb[i].pid, pcb[i].tid);
                    j++;
                }
            }else{
                pid_t mem_tid = pcb[i].tid_mem;
                if(tcb[mem_tid].status == TASK_RUNNING){
                    prints("[%d] PID : %d  TID : %d STATUS : RUNNING\n",j ,pcb[i].pid, tcb[mem_tid].tid);
                    j++;
                }else if(tcb[mem_tid].status == TASK_READY){
                    prints("[%d] PID : %d  TID : %d STATUS : READY\n", j ,pcb[i].pid ,tcb[mem_tid].tid);
                    j++;
                }else if(tcb[mem_tid].status == TASK_BLOCKED){
                    prints("[%d] PID : %d  TID : %d STATUS : BLOCKED\n",j , pcb[i].pid, tcb[mem_tid].tid);
                    j++;
                }
            }
        }
    }
}

//Exec Process ---> sys_spawn(). We Add New Process(Thread) to Our Ready Queue
pid_t do_spawn(task_info_t *task, void* arg, spawn_mode_t mode){
    pcb_t *new_pcb;
    /*First, we need to judge if exit_queue is empty, otherwise we can reuse its resources*/
    if(!list_empty(&exit_queue)){
        new_pcb = list_entry(exit_queue.next, pcb_t, list);
        list_del(exit_queue.next);  
    }else{
        new_pcb = &pcb[process_num++];
    }

    new_pcb -> kernel_sp = allocPage(1) + PAGE_SIZE;
    new_pcb -> user_sp   = allocPage(1) + PAGE_SIZE;
    new_pcb -> type = task -> type;

    init_pcb_stack(new_pcb -> kernel_sp, new_pcb -> user_sp, task -> entry_point, arg, NULL,  new_pcb);
    new_pcb -> kernel_sp = new_pcb -> kernel_sp - sizeof(switchto_context_t) - sizeof(regs_context_t);
    list_add_tail(&(new_pcb -> list), &ready_queue);
    
    new_pcb -> preempt_count = 0;
    new_pcb -> status = TASK_READY;
    new_pcb -> tick = 0;

    new_pcb -> pid = process_id;
    process_id ++;

    new_pcb -> cursor_x = new_pcb -> cursor_y = 0;
    new_pcb -> lock_num = 0;
    
    //Init Wait List to Do Sys_waitpid()
    init_list_head(&(new_pcb -> wait_list));

    return new_pcb -> pid;
}

//Kill a Process ----> sys_kill(). Recycle its resources.
int do_kill(pid_t pid, pid_t tid){
    int i = 0;
    //Find the PCB
    if(pid == 0 || pid == 1){
        return 0;
    }

    for(i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].pid == pid){
            break;
        }
    }

    //No Target PCB. Do Not Find. Return 0
    if(i == NUM_MAX_TASK){
        return 0;
    }

    pcb_t *killed_pcb;
    if(tid == 0){
        killed_pcb = &pcb[i];
    }else{
        killed_pcb = &tcb[pcb[i].tid_mem];
    }

    //We Delete All PCB List. Whether this process is sleeping or ruuning
    if(killed_pcb -> status == TASK_READY || killed_pcb -> status == TASK_BLOCKED){
        list_del(&(killed_pcb -> list));        
    }
    //Release the LOCK!
    for(i = 0; i < killed_pcb -> lock_num; i++){
        mthread_semaphore *p = killed_pcb -> lock[i];
        if(!list_empty(&(p -> wait_queue))){
        //Only when block_queue is not empty, we need to release
          pcb_t *she = list_entry((&(p -> wait_queue)) -> next, pcb_t, list);
          she -> status = TASK_READY;
          list_del((&(p -> wait_queue)) -> next);
          list_add_tail(&(she -> list), &ready_queue);
        //If the queue is not empty, the status is locked
        }else{
        //If the queue is empty. The status is unlocked
            p -> s = 1;
        }
    }

    //Delete All lists in Wait Queue
    while(!list_empty(&killed_pcb -> wait_list)){
        pcb_t *p;
        p = list_entry(killed_pcb -> wait_list.next, pcb_t, list);
        if(p -> status == TASK_BLOCKED){
            p -> status = TASK_READY;
            list_del(&(p -> list));
            list_add_tail(&(p -> list), &ready_queue);
        }
    }

    //killed_pcb -> pid = 0;
    killed_pcb -> status = TASK_EXITED;

    //ADD TO THE EXIT_QUEUE
    //list_add_tail(&(pcb -> list), &exit_queue);
    long cpu_id = get_current_cpu_id();
    
    list_node_t *head = &(killed_pcb -> plist);
    
      while(!list_empty(&(killed_pcb -> plist))){
          page_t *p = list_entry(head -> next, page_t, list);
          freePage(p -> pa);
          list_del(head -> next);
      }
    
    //Do Not Forget to Do Schedule
    if(killed_pcb == current_running[cpu_id]){
        do_scheduler();
    }
    //Successfully Kill
    return 1;
}

//Process Exit. Warning: There are two modes! ---> sys_exit().
void do_exit(void){
    long cpu_id = get_current_cpu_id();
    pcb_t *exited_pcb = current_running[cpu_id];

    //Release the LOCK!
    /*for(int i = 0; i < exited_pcb -> lock_num; i++){
        do_mutex_lock_release(exited_pcb -> lock[i]);
    }*/
    exited_pcb -> pid = 0;

    //Delete the PCB in Wait Queue
    while(!list_empty(&exited_pcb -> wait_list)){
        pcb_t *p;
        p = list_entry(exited_pcb -> wait_list.next, pcb_t, list);
        if(p -> status == TASK_BLOCKED){
            p -> status = TASK_READY;
            list_del(&(p -> list));
            list_add_tail(&(p -> list), &ready_queue);
        }
    }


    //We Set Different Status According to the Mode
    if(exited_pcb -> mode == AUTO_CLEANUP_ON_EXIT){
        exited_pcb -> status = TASK_EXITED;
        //list_add_tail(&(exited_pcb -> list), &exit_queue);
    }else if(exited_pcb -> mode == ENTER_ZOMBIE_ON_EXIT){
        exited_pcb -> status = TASK_ZOMBIE;
    }

    list_node_t *head = &(exited_pcb -> plist);
    while(!list_empty(&(exited_pcb -> plist))){
        page_t *p = list_entry(head -> next, page_t, list);
        freePage(p -> pa);
        list_del(head -> next);
    }
    
    //Don't forget to do schedule()
    do_scheduler();
}

//WaitPid. Current_running cannot run until pid end! ---> sys_waitpid()
int do_waitpid(pid_t pid){
    if(pid == 1 || pid  == 0){
        return 0;
    }
    int i = 0;
    
    //Find the PCB
    for(i = 0; i < NUM_MAX_TASK; i++){
        if(pcb[i].pid == pid){
            break;
        }
    }
    //Do not find
    if(i == NUM_MAX_TASK){
        return 0;
    }
    long cpu_id = get_current_cpu_id();
    pcb_t *wait_pcb = &pcb[i];
    //Block the current PCB into the pid's wait_queue
    list_add_tail(&(current_running[cpu_id] -> list), &(wait_pcb -> wait_list));
    current_running[cpu_id] -> status = TASK_BLOCKED;
    //Do Not Forget to Do Schedule!
    do_scheduler();
    return 1;
}

//Get PID: Get Current Running's PID ---> sys_getpid()
pid_t do_getpid(){
    long cpu_id = get_current_cpu_id();
    pid_t pid1 = current_running[cpu_id] -> pid;
    return pid1;
}


int match(char *file_name){
    for(int i = 0; i < ELF_FILE_NUM; i++){
        if(!strcmp(file_name, elf_files[i].file_name)){
            return i;
        }
    }
    return -1;
}

//Prj4 ----> modified_do_exec
pid_t do_exec(const char* file_name, int argc, char* argv[], spawn_mode_t mode){

    int match_status = match(file_name);
    //If we do not find this file
    if(match_status == -1){
        return 0;
    }

    long cpu_id = get_current_cpu_id();
    pcb_t *new_pcb = &pcb[process_num++];
    for(int l = 0; l < sizeof(pcb_t); l++){
        *((char*)(new_pcb) + l) = 0;
    }
    //Some other fields in prj2
    new_pcb -> preempt_count = 0;
    new_pcb -> pid = process_id;
    process_id++;
    new_pcb -> cursor_x = new_pcb -> cursor_y = 0;
    new_pcb -> tick = 0;
    new_pcb -> status = TASK_READY;
    new_pcb -> type = USER_PROCESS;
    new_pcb -> lock_num = 0;
    new_pcb -> mode = mode;
    new_pcb -> tid = 0;
    tid[new_pcb -> pid] = 1;
    new_pcb -> pgdir = allocPage();
    //Copy the kernel page, note: we use the va
    memcpy((char*)new_pcb -> pgdir, (char*)pa2kva(PGDIR_PA), PAGE_SIZE);
    //Kernel_stack & User_stack, we finally get va + PAGE_SZIE.
    ptr_t kernel_sp = alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, new_pcb -> pgdir, 0) + PAGE_SIZE;
    ptr_t user_sp   = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, new_pcb -> pgdir, 1) + PAGE_SIZE - 0xf0;
    
    uintptr_t entry = load_elf(elf_files[match_status].file_content,
                               *elf_files[match_status].file_length,
                                new_pcb -> pgdir,
                                alloc_page_helper_t);

    //Copy
    uintptr_t argvn = USER_STACK_ADDR - 0xf0;
    uint64_t *kargv = user_sp;      //Kernel
    for(int k = 0; k < argc; k++){
        *(kargv + k) = (uint64_t)(argvn + 0x10*(k+1));     //128 bits align
        memcpy((char*)(user_sp + 0x10*(k+1)), argv[k], kstrlen(argv[k])+1);
    }

    init_pcb_stack(kernel_sp, user_sp, entry, argc, argvn, new_pcb);

    new_pcb -> pgdir = kva2pa(new_pcb -> pgdir);
    init_list_head(&(new_pcb -> wait_list));
    init_list_head(&(new_pcb -> plist));
    list_add_tail(&(new_pcb -> list), &ready_queue);
    
    page_t *kstack = (page_t*)kmalloc(sizeof(page_t));
    kstack -> pa = kva2pa(kernel_sp - PAGE_SIZE);
    kstack -> va = KERNEL_STACK_ADDR - PAGE_SIZE;
    kstack -> atds = 1;
    init_list_head(&(kstack -> list));
    page_t *ustack = (page_t*)kmalloc(sizeof(page_t));
    ustack -> pa = kva2pa(user_sp + 0xf0 - PAGE_SIZE);
    ustack -> va = USER_STACK_ADDR - PAGE_SIZE;
    ustack -> atds = 1;
    init_list_head(&(ustack -> list));

    list_add_tail(&(kstack -> list), &(new_pcb -> plist));
    list_add_tail(&(ustack -> list), &(new_pcb -> plist));
    new_pcb -> pnum = 2;
    return new_pcb -> pid;
}

void do_show_exec(){
    for(int i = 1; i < ELF_FILE_NUM; i++){
        prints("%s ", elf_files[i].file_name);
    }
    prints("\n");
}

//Mthread function


pid_t thread_id = 1;
int do_mthread_create(pid_t *thread, void (*start_routine)(void*), char *argv[]){
    long cpu_id = get_current_cpu_id();

    *thread = tid[current_running[cpu_id] -> pid];
    tid[current_running[cpu_id] -> pid]++;
    pcb_t *new_tcb = &tcb[thread_id];
    current_running[cpu_id] -> tid_mem = thread_id;
    new_tcb -> preempt_count = 0;
    new_tcb -> pid = current_running[cpu_id] -> pid;
    new_tcb -> cursor_x = new_tcb -> cursor_y = 0;
    new_tcb -> tick = 0;
    new_tcb -> status = TASK_READY;
    new_tcb -> type = USER_PROCESS;
    new_tcb -> tid = *thread;
    thread_id++;
    new_tcb -> mode = current_running[cpu_id] -> mode;
    //Share the pgdir
    new_tcb -> pgdir = pa2kva(current_running[cpu_id] -> pgdir);

    
    ptr_t kernel_sp = alloc_page_helper(KERNEL_STACK_ADDR + thread_id*PAGE_SIZE - PAGE_SIZE, new_tcb -> pgdir, 0) + PAGE_SIZE;
    ptr_t user_sp   = alloc_page_helper(USER_STACK_ADDR + thread_id*PAGE_SIZE - PAGE_SIZE, new_tcb -> pgdir, 1) + PAGE_SIZE - 0xf0;

    uintptr_t entry = start_routine;
    //copy
    uintptr_t argvn = USER_STACK_ADDR + thread_id*PAGE_SIZE - 0xf0;
    uint64_t *kargv = user_sp;      //Kernel
    for(int k = 0; k < 2; k++){
        *(kargv + k) = (uint64_t)(argvn + 0x10*(k+1));     //128 bits align
        memcpy((char*)(user_sp + 0x10*(k+1)), (char*)argv[k], kstrlen(argv[k])+1);
    }
    init_tcb_stack(kernel_sp, user_sp, entry, 1, argvn, thread_id*PAGE_SIZE + USER_STACK_ADDR ,new_tcb);
    new_tcb -> pgdir = current_running[cpu_id] -> pgdir;
    init_list_head(&(new_tcb -> list));
    init_list_head(&(new_tcb -> wait_list));
    //
    init_list_head(&(new_tcb -> plist));
    page_t *kstack = (page_t*)kmalloc(sizeof(page_t));
    kstack -> pa = kva2pa(kernel_sp - PAGE_SIZE);
    kstack -> va = KERNEL_STACK_ADDR + thread_id*PAGE_SIZE - PAGE_SIZE;
    kstack -> atds = 1;
    init_list_head(&(kstack -> list));
    list_add_tail(&(kstack -> list), &(new_tcb -> plist));
    //
    list_add_tail(&(new_tcb -> list), &ready_queue);
    return new_tcb -> tid;
}
