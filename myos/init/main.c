/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *         The kernel's entry, where most of the initialization work is done.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
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

#include <common.h>
#include <os/irq.h>
#include <os/mm.h>
#include <os/sched.h>
#include <screen.h>
#include <sbi.h>
#include <stdio.h>
#include <os/time.h>
#include <os/syscall.h>
#include <assert.h>
//#include <test.h> 
//add macro
#include <os/list.h>
#include <os/lock.h>
#include <csr.h>
#include <os/smp.h>
#include <../include/os/elf.h>
#include <../include/user_programs.h>

#include <plic.h>
#include <emacps/xemacps_example.h>
#include <net.h>

#include <os/fs.h>

#define KERNEL 0
#define USER   1

extern void ret_from_exception();
extern void print_task1(void);
extern void __global_pointer$();

void  init_pcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, void *arg, char *argv[],
    pcb_t *pcb)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    //printk("%ld",pt_regs);
    sbi_console_putchar('h');  
    /* TODO: initialization registers
     * note: sp, gp, ra, sepc, sstatus
     * gp should be __global_pointer$
     * To run the task in user mode,
     * you should set corresponding bits of sstatus(SPP, SPIE, etc.).
     */
    int i;
    /*Initial all registers 0*/
    /*for(i = 0; i < 32; i++){
        pt_regs -> regs[i] = 0;
    }*/
      
    //if user mode, we need to initiallize some special registers
    pt_regs -> regs[2] = USER_STACK_ADDR - 0xf0;   //sp pointer
    //sbi_console_putchar('i');   
    pt_regs -> regs[3] = (reg_t)__global_pointer$; //gp pointer
    pt_regs -> regs[1] = (reg_t)entry_point;       //ra pointer
    pt_regs -> regs[4] = (reg_t)pcb;               //tp pointer
    pt_regs -> sepc    = (reg_t)entry_point;       //sepc
    pt_regs -> regs[10] = (reg_t)arg; 
    pt_regs -> regs[11] = (reg_t)argv;
    pt_regs -> sstatus =  (1 << 18);         //sstatus SUM should be set 1 in prj4 
    // set sp to simulate return from switch_to
    /* TODO: you should prepare a stack, and push some values to
     * simulate a pcb context.
     */
    //Below is to simulate PT_REGS to alloc
    
    switchto_context_t *st_regs = 
        (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));

    //Only in User Mode we need to switch mode!!!
     //sbi_console_putchar('j');   
    if(pcb -> type == USER_PROCESS || pcb -> type == USER_THREAD){
        st_regs -> regs[0] = &ret_from_exception;                                                       //ra
    }else{
        st_regs -> regs[0] = entry_point;
    }
    //kernel need to load regs_contex and switch_to_contex, sp points the bottom of this stack
     //sbi_console_putchar('k');   
    st_regs -> regs[1] = (reg_t)USER_STACK_ADDR - 0xf0;                                                            //sp
    pcb -> kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb -> user_sp   = USER_STACK_ADDR - 0xf0;
}


void init_tcb_stack(
    ptr_t kernel_stack, ptr_t user_stack, ptr_t entry_point, void *arg, char *argv[], uintptr_t user, 
    pcb_t *pcb)
{
    regs_context_t *pt_regs =
        (regs_context_t *)(kernel_stack - sizeof(regs_context_t));
    
    /* TODO: initialization registers
     * note: sp, gp, ra, sepc, sstatus
     * gp should be __global_pointer$
     * To run the task in user mode,
     * you should set corresponding bits of sstatus(SPP, SPIE, etc.).
     */
    long cpu_id = get_current_cpu_id();
    int i;
    /*Initial all registers 0*/
    /*for(i = 0; i < 32; i++){
        pt_regs -> regs[i] = 0;
    }*/
    regs_context_t *father_pt_regs = (regs_context_t*)((current_running[cpu_id] -> kernel_sp) - sizeof(regs_context_t)); 
    //if user mode, we need to initiallize some special registers
    pt_regs -> regs[2] = user - 0xf0;              //sp pointer
    pt_regs -> regs[3] = father_pt_regs -> regs[3];        //gp pointer
    pt_regs -> regs[1] = (reg_t)entry_point;       //ra pointer
    pt_regs -> regs[4] = (reg_t)pcb;               //tp pointer
    pt_regs -> sepc    = (reg_t)entry_point;       //sepc
    pt_regs -> regs[10] = (reg_t)argv; 
    //pt_regs -> regs[11] = (reg_t)argv;
    pt_regs -> sstatus =  (1 << 18);         //sstatus SUM should be set 1 in prj4 
    // set sp to simulate return from switch_to
    /* TODO: you should prepare a stack, and push some values to
     * simulate a pcb context.
     */
    //Below is to simulate PT_REGS to alloc
    switchto_context_t *st_regs = 
        (switchto_context_t *)(kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t));

    //Only in User Mode we need to switch mode!!!
    if(pcb -> type == USER_PROCESS || pcb -> type == USER_THREAD){
        st_regs -> regs[0] = &ret_from_exception;                                                       //ra
    }else{
        st_regs -> regs[0] = entry_point;
    }
    //kernel need to load regs_contex and switch_to_contex, sp points the bottom of this stack
    st_regs -> regs[1] = (reg_t)user - 0xf0;                                                            //sp
    pcb -> kernel_sp = kernel_stack - sizeof(regs_context_t) - sizeof(switchto_context_t);
    pcb -> user_sp   = user - 0xf0;
}

/*------------------------------P3 INIT SHELL PCB---------------------------------------*/
static void init_shell(void){
    /*  kernel_sp ---> VA
     *  user_sp   ---> VA                      
     *  pgdir     ---> VA    */ 
    sbi_console_putchar('a');
    init_list_head(&ready_queue);  
    long cpu_id = get_current_cpu_id();
     for(int l = 0; l < sizeof(pcb_t); l++){
        *((char*)(&pcb[0]) + l) = 0;
    }
    //Some other fields in prj2
    pcb[0].preempt_count = 0;
    pcb[0].pid = 1;
    pcb[0].cursor_x = pcb[0].cursor_y = 0;
    pcb[0].tick = 0;
    pcb[0].status = TASK_READY;
    pcb[0].type = USER_PROCESS;
    //Alloc a pgdir, return the pgdir's va
    pcb[0].pgdir = allocPage();
    sbi_console_putchar('b');    
    //Copy the kernel page, note: we use the va
    memcpy((char*)pcb[0].pgdir, (char*)pa2kva(PGDIR_PA), PAGE_SIZE);
    sbi_console_putchar('c');   
    //Kernel_stack & User_stack, we finally get va + PAGE_SZIE.
    ptr_t kernel_sp = alloc_page_helper(KERNEL_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, KERNEL) + PAGE_SIZE;
    sbi_console_putchar('d');   
    ptr_t user_sp   = alloc_page_helper(USER_STACK_ADDR - PAGE_SIZE, pcb[0].pgdir, USER) + PAGE_SIZE - 0xf0;
    sbi_console_putchar('e');   
    //Load elf to get entry addr
    uintptr_t entry = load_elf(_elf___test_test_shell_elf,
                               _length___test_test_shell_elf, pcb[0].pgdir
                                ,alloc_page_helper_t);
    sbi_console_putchar('f'); 
    init_pcb_stack(kernel_sp, user_sp, entry, NULL, NULL, &pcb[0]);
    sbi_console_putchar('g');   
    pcb[0].pgdir = kva2pa(pcb[0].pgdir);  
     //sbi_console_putchar('g');   
    //pcb[0].kernel_sp = pcb[0].kernel_sp - sizeof(switchto_context_t) - sizeof(regs_context_t);
    list_add_tail(&pcb[0].list, &ready_queue);
    init_list_head(&send_queue);
    init_list_head(&recv_queue);
    
    //Remember to Set Current Running 
    current_running[0] = &pid0_pcb_0;
}
/*------------------------------P3 INIT SHELL PCB END------------------------------------*/

static void init_pcb()
{
     /* initialize all of your pcb and add them into ready_queue
     * TODO:
     */
    init_list_head(&ready_queue);   //init ready queue
    pcb_t pcb[16];                  //all pcbs
    pid_t pid = 2;                  //pid0 is 0, pid1 is shell
    //Initialize all pcbs in task 3 & 4
    int i;
    /*for(i = 0; i < ; i++){
        pcb[i].kernel_sp = allocPage(1) + PAGE_SIZE;                 //alloc 1 page(4k)
        pcb[i].user_sp   = allocPage(1) + PAGE_SIZE;                 //alloc 1 page(4k)
        //To init registers and kernel_sp and user_sp
        pcb[i].type = timer_tasks[i] -> type;    
        init_pcb_stack(pcb[i].kernel_sp, pcb[i].user_sp, timer_tasks[i] -> entry_point, &pcb[i]);
        
        pcb[i].kernel_sp = pcb[i].kernel_sp - sizeof(switchto_context_t) - sizeof(regs_context_t);

        pcb[i].preempt_count = 0;                        //preempt conut initialize 0

        //add pcb list into queue(list_add_tail suit to realize queue)
        list_add_tail(&pcb[i].list,&ready_queue);        
        pcb[i].pri = timer_tasks[i] -> pri;

        pcb[i].pid = pid;
        pid ++;
                         //TASK 1 all KERNEL_THREAD(test.c)
        pcb[i].tick = 0;
        pcb[i].status = TASK_READY;                      //All PCB which are initialized should in READY STATE
        
        pcb[i].cursor_x = pcb[i].cursor_y = 0;           //cursor poition are both 0
    }*/


    /* remember to initialize `current_running`
     * TODO:
     */
    //current_running = &pid0_pcb;                        //current_running is pcb0
}

static void init_syscall(void)
{
    // initialize system call table.
    //If we cannot find the syscall number, it will handle other
    for(int i = 0; i < NUM_SYSCALLS; i++){
        syscall[i] = &handle_other;
    }
    //Below we want to set the trap name 
    syscall[SYSCALL_SLEEP] = &do_sleep;
    syscall[SYSCALL_YIELD] = &do_scheduler;
    syscall[SYSCALL_WRITE] = /*port_write;*/&screen_write;
    syscall[SYSCALL_READ]  = &sbi_console_getchar;
    syscall[SYSCALL_CURSOR]= /*&vt100_move_cursor;*/&screen_move_cursor;
    syscall[SYSCALL_REFLUSH] = &screen_reflush;
    syscall[SYSCALL_GET_TIMEBASE] = &get_time_base;
    syscall[SYSCALL_GET_TICK] = &get_ticks;
    //Below are syscalls related to lock 
    syscall[SYSCALL_MUTEX_LOCK_INIT] = &do_mutex_lock_init;
    syscall[SYSCALL_MUTEX_LOCK_ACQUIRE] = &do_mutex_lock_acquire;
    syscall[SYSCALL_MUTEX_LOCK_UNBLOCK] = &do_mutex_lock_release;
    //syscall[SYSCALL_GET_WALL_TIME] = &get_wall_time;
    //syscall[SYSCALL_FORK] = &fork;

    /*------------Prj 3 New Syscall!!----------------*/
    syscall[SYSCALL_GET_CHAR] = &sbi_console_getchar;
    syscall[SYSCALL_PUT_CHAR] = &sbi_console_putchar;
    syscall[SYSCALL_PS]       = &do_process_show;
    syscall[SYSCALL_SCREEN_CLEAR] = &screen_clear;
    syscall[SYSCALL_SPAWN] = &do_spawn;
    syscall[SYSCALL_KILL] = &do_kill;
    syscall[SYSCALL_EXIT] = &do_exit;
    syscall[SYSCALL_WAITPID] = &do_waitpid;
    syscall[SYSCALL_GETPID] = &do_getpid;
    
    syscall[SYSCALL_SEM_INIT] = &do_sem_init;
    syscall[SYSCALL_SEM_UP] = &do_sem_up;
    syscall[SYSCALL_SEM_DOWN] = &do_sem_down;
    syscall[SYSCALL_SEM_DESTROY] = &do_sem_destroy;

    syscall[SYSCALL_BARRIER_INIT] = &do_barrier_init;
    syscall[SYSCALL_BARRIER_WAIT] = &do_barrier_wait;
    syscall[SYSCALL_BARRIER_DESTROY] = &do_barrier_detroy;

    syscall[SYSCALL_MBOX_OPEN] = &do_mbox_open;
    syscall[SYSCALL_MBOX_CLOSE] = &do_mbox_close;
    syscall[SYSCALL_MBOX_SEND] = &do_mbox_send;
    syscall[SYSCALL_MBOX_RECV] = &do_mbox_recv;
    /*prj4*/
    syscall[SYSCALL_EXEC] = &do_exec;
    syscall[SYSCALL_SHOW_EXEC] = &do_show_exec;
    syscall[SYSCALL_BINSEMGET] = &do_binsem_get;
    syscall[SYSCALL_BINSEMOP] = &do_binsem_op;
    syscall[SYSCALL_MTHREAD_CREATE] = &do_mthread_create;
    /*prj5*/
    syscall[SYSCALL_NET_SEND] = &do_net_send;
    syscall[SYSCALL_NET_RECV] = &do_net_recv;
    syscall[SYSCALL_NET_IRQ_MODE] = &do_net_irq_mode;
    /*prj6*/
    syscall[SYSCALL_MKFS] = &do_mkfs;
    syscall[SYSCALL_STATFS] = &do_statfs;
    syscall[SYSCALL_MKDIR] = &do_mkdir;
    syscall[SYSCALL_CD] = &do_cd;
    syscall[SYSCALL_LS] = &do_ls;
    syscall[SYSCALL_RMDIR] = &do_rmdir;
    syscall[SYSCALL_TOUCH] = &do_touch;
    syscall[SYSCALL_FOPEN] = &do_fopen;
    syscall[SYSCALL_FREAD] = &do_fread;
    syscall[SYSCALL_FCLOSE] = &do_fclose;
    syscall[SYSCALL_FWRITE] = &do_fwrite;
    syscall[SYSCALL_CAT] = &do_cat;
    syscall[SYSCALL_LSEEK] = &do_lseek;
    syscall[SYSCALL_LN] = &do_ln;
    syscall[SYSCALL_RM] = &do_rm;
    syscall[SYSCALL_LS_L] = &do_ls_l;
}



void cancel_mapping()
{
    uint64_t va = 0x50200000;
    uint64_t pgdir = 0xffffffc05e000000;
    uint64_t vpn_2 = (va >> 30) & ~(~0 << 9);
    uint64_t vpn_1 = (va >> 21) & ~(~0 << 9);
    PTE *level_2 = (PTE *)pgdir + vpn_2;
    PTE *final_pte = (PTE *)pa2kva(get_pa(*level_2)) + vpn_1;
    *level_2 = 0;
    *final_pte = 0;
}


void boot_first_core(unsigned long mhartid, uintptr_t _dtb)
{

    cancel_mapping();
    init_shell();
    printk("> [INIT] Shell initialization succeeded.\n\r");
    // setup timebase
    // fdt_print(_dtb);
    // get_prop_u32(_dtb, "/cpus/cpu/timebase-frequency", &time_base);
    time_base = sbi_read_fdt(TIMEBASE);
    uint32_t slcr_bade_addr = 0, ethernet_addr = 0;

    // get_prop_u32(_dtb, "/soc/slcr/reg", &slcr_bade_addr);
    slcr_bade_addr = sbi_read_fdt(SLCR_BADE_ADDR);
    printk("[slcr] phy: 0x%x\n\r", slcr_bade_addr);

    // get_prop_u32(_dtb, "/soc/ethernet/reg", &ethernet_addr);
    ethernet_addr = sbi_read_fdt(ETHERNET_ADDR);
    printk("[ethernet] phy: 0x%x\n\r", ethernet_addr);

    uint32_t plic_addr = 0;
    // get_prop_u32(_dtb, "/soc/interrupt-controller/reg", &plic_addr);
    plic_addr = sbi_read_fdt(PLIC_ADDR);
    printk("[plic] plic: 0x%x\n\r", plic_addr);

    uint32_t nr_irqs = sbi_read_fdt(NR_IRQS);
    // get_prop_u32(_dtb, "/soc/interrupt-controller/riscv,ndev", &nr_irqs);
    printk("[plic] nr_irqs: 0x%x\n\r", nr_irqs);

    XPS_SYS_CTRL_BASEADDR =
        (uintptr_t)ioremap((uint64_t)slcr_bade_addr, NORMAL_PAGE_SIZE);
    xemacps_config.BaseAddress =
        (uintptr_t)ioremap((uint64_t)ethernet_addr, NORMAL_PAGE_SIZE);
    uintptr_t _plic_addr =
        (uintptr_t)ioremap((uint64_t)plic_addr, 0x4000*NORMAL_PAGE_SIZE);
    // XPS_SYS_CTRL_BASEADDR = slcr_bade_addr;
    // xemacps_config.BaseAddress = ethernet_addr;
    xemacps_config.DeviceId        = 0;
    xemacps_config.IsCacheCoherent = 0;

    printk(
        "[slcr_bade_addr] phy:%x virt:%lx\n\r", slcr_bade_addr,
        XPS_SYS_CTRL_BASEADDR);
    printk(
        "[ethernet_addr] phy:%x virt:%lx\n\r", ethernet_addr,
        xemacps_config.BaseAddress);
    printk("[plic_addr] phy:%x virt:%lx\n\r", plic_addr, _plic_addr);
    plic_init(_plic_addr, nr_irqs);
    long status = EmacPsInit(&EmacPsInstance);
    if (status != XST_SUCCESS) {
        printk("Error: initialize ethernet driver failed!\n\r");
        assert(0);
    }

    // init futex mechanism
    //init_system_futex();

    // init interrupt (^_^)
    init_exception();
    printk(
        "> [INIT] Interrupt processing initialization "
        "succeeded.\n\r");

    // init system call table (0_0)
    init_syscall();
    printk("> [INIT] System call initialized successfully.\n\r");

    // enable_interrupt();
    net_poll_mode = 1;
    // xemacps_example_main();

    // init screen (QAQ)
    init_screen();
    printk("> [INIT] SCREEN initialization succeeded.\n\r");
    // screen_clear(0, SCREEN_HEIGHT - 1);
}
// jump from bootloader.
// The beginning of everything >_< ~~~~~~~~~~~~~~
int main(unsigned long mhartid, uintptr_t _dtb)
{
    long cpu_id;
    cpu_id = get_current_cpu_id();
    
    if(cpu_id == 0){
        //init_syscall();
        // init Process Control Block (-_-!)
        cancel_mapping();
        init_shell();
        printk("> [INIT] Shell initialization succeeded.\n\r");
        
        // read CPU frequency
        time_base = sbi_read_fdt(TIMEBASE);

        // init interrupt (^_^)
        init_exception();
        printk("> [INIT] Interrupt processing initialization succeeded.\n\r");

        // init system call table (0_0)
        init_syscall();
        printk("> [INIT] System call initialized successfully.\n\r");

        // fdt_print(riscv_dtb);

        // init screen (QAQ)
        init_screen();
        printk("> [INIT] SCREEN initialization succeeded.\n\r");

        sbi_sd_read(SB_MEM_ADDR, 1, FS_START);
        superblock_t *sp = (superblock_t*)(pa2kva(SB_MEM_ADDR));
        if(sp -> magic != MAGIC){
            do_mkfs();
        }
        //boot_first_core(mhartid, _dtb);
        printk("Finish Init Core[%ld]\n",cpu_id);
        smp_init();
        lock_kernel();
        //wakeup_other_hart();
    }else{
        //smp_init();
        lock_kernel();
        //lock_kernel();
        current_running[1] = &pid0_pcb_1; 
        setup_exception(); 
        
        //unlock_kernel();
        printk("Finish Init Core[1]\n");
    }


     // TODO:
    // Setup timer interrupt and enable all interrupt
    reset_irq_timer();
    unlock_kernel();
    enable_interrupt();

    while (1) {
        // (QAQQQQQQQQQQQ)
        // If you do non-preemptive scheduling, you need to use it
        // to surrender control do_scheduler();
        // enable_interrupt();
        // __asm__ __volatile__("wfi\n\r":::);
        //do_scheduler();
    };
    return 0;
}
