

#include <os/irq.h>
#include <os/time.h>
#include <os/sched.h>
#include <os/string.h>
#include <stdio.h>
#include <assert.h>
#include <sbi.h>
#include <screen.h>
#include <pgtable.h>

#include <os/mm.h>
#include <net.h>
#include <emacps/xemacps_example.h>
#include <plic.h>


handler_t irq_table[IRQC_COUNT];
handler_t exc_table[EXCC_COUNT];
uintptr_t riscv_dtb;

int swap_block = 10000;
int print_location = 10;

//Mem write to Disk
void mem_to_disk(int block_id, int block_num, uintptr_t pa, uintptr_t va){
    sbi_sd_write(pa, block_num, block_id);
    screen_move_cursor(1,print_location++);
    prints("Successfully Swap from MEM to SD Block");
    //printk("\n");
    screen_reflush();
}

void disk_to_mem(int block_id, int block_num, uintptr_t pa, uintptr_t va){
    sbi_sd_read(pa, block_num, block_id);
    screen_move_cursor(1,print_location++);
    prints("Successfully Swap from SD BLOCK to MEM");
    //printk("\n");
    screen_reflush();
}

//check_recv
void recv_check(void)
{
    list_node_t *current_node;
    pcb_t *p;
    current_node = recv_queue.next;
    int recv_num = 0;
    if(!list_empty(&recv_queue)){
        p = list_entry(current_node, pcb_t, list);       //get the pcb
        while(bd_space[recv_num] & 0x1 == 1){
            recv_num ++;
        }
        if(bd_space[recv_num - 1] & 0x3 == 3){
            pcb_t *unblock_pcb;
            unblock_pcb = list_entry(current_node, pcb_t, list);
            unblock_pcb -> status = TASK_READY;
            list_del(current_node);
            list_add_tail(current_node, &ready_queue);
        }
    }
}

//check_send
void send_check(void)
{
    list_node_t *current_node;
    pcb_t *p;
    current_node = send_queue.next;
    int recv_num = 0;
    if(!list_empty(&send_queue)){
        int received = bd_space[0x40] >> 63;
        if(received){
            pcb_t *unblock_pcb;
            unblock_pcb = list_entry(current_node, pcb_t, list);
            unblock_pcb -> status = TASK_READY;
            list_del(current_node);
            list_add_tail(current_node, &ready_queue);
        }
    }
}

//Swap from mem to disk
int do_swap(pcb_t *pcb){

    if(list_empty(&(pcb -> plist))){
        return 0;
    }
    list_node_t *p = pcb -> plist.next;
    page_t *swap_page_t = NULL;
    //FIFO ALGORITHM
    while(p != &(pcb -> plist)){
        swap_page_t = list_entry(p, page_t, list);
        uintptr_t *pte = check_ad(swap_page_t -> va, pa2kva(pcb -> pgdir));    
        //Invalid
        if(pte == 0){
            //printk("This page to swap is invalid!\n\r");
            return 0;
        }
        //If not in disk
        if(!swap_page_t -> atds){
            //Write to disk
            mem_to_disk(swap_block, 8, swap_page_t -> pa, swap_page_t -> va);
            swap_page_t -> atds = 1;
            swap_page_t -> pte = pte;
            swap_page_t -> block_id = swap_block;
            swap_block += 8;
            *pte &= ~((uint64_t)0x1);       //Set valid field 0
            pcb -> pnum --;
            return 1;
        }
        p = p -> next;
    }
    //Do not Find
    printk("There is no Page to Swap!\n");
    return 0;
}

//Swap from disk to mem
int do_unswap(pcb_t *pcb, uintptr_t va){
    list_node_t *p = pcb -> plist.next; 
    page_t *unswap_page_t = NULL;

    while(p != &(pcb -> plist)){
        unswap_page_t = list_entry(p, page_t, list);
        if(va == unswap_page_t -> va && unswap_page_t -> atds == 1){
            disk_to_mem(unswap_page_t -> block_id, 8, unswap_page_t -> pa, unswap_page_t -> va);
            unswap_page_t -> atds = 0;
            uintptr_t *pte = unswap_page_t -> pte;
            set_attribute(pte, 0x1);
            unswap_page_t -> block_id = 0;
            pcb -> pnum ++;
            //Move p to last
            list_del(p);
            list_add_tail(p, &(pcb -> plist));
            return 1;
        }
        p = p -> next;
    }
    //printk("[>Error] Unswap Operation is Not Successful!\n");
    return 0;
}

void reset_irq_timer()
{
    // TODO clock interrupt handler.
    // TODO: call following functions when task4
    screen_reflush();
    timer_check();
    //recv_check();
    //send_check();
    sbi_set_timer(get_ticks() + get_time_base()/100);
    // note: use sbi_set_timer
    // remember to reschedule
    do_scheduler();
}

void interrupt_helper(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // TODO interrupt handler.
    // call corresponding handler by the value of `cause`
    //printk("cause: %d\n", cause >> 63);
    handler_t *table = (cause >> 63)? irq_table: exc_table;
    cause = cause << 1;
    cause = cause >> 1;                 //Aim to clear the top bit
    table[cause](regs, stval, cause);   //Invoke function
}

void handle_int(regs_context_t *regs, uint64_t interrupt, uint64_t cause)
{
    reset_irq_timer();
}

// !!! NEW: handle_irq
extern uint64_t read_sip();
void handle_irq(regs_context_t *regs, int irq)
{
    // TODO: 
    // handle external irq from network device
    printk("Start handle irq!\n");
    u32 isr_reg;
    isr_reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET);
    //Recv simulate time irq
    int recv_num = 0; 
    list_node_t *current_node_recv;
    current_node_recv = recv_queue.next;
    //int recv_num = 0;
    if(isr_reg & XEMACPS_IXR_FRAMERX_MASK){
        //printk("Start Handling rx irq!\n");
        while(bd_space[recv_num] & 0x1 == 1){
            recv_num++;
        }
        //judge if last?
        if(bd_space[recv_num] & 0x3 == 3){
            if(!list_empty(&recv_queue)){
                pcb_t *unblock_pcb_recv;
                unblock_pcb_recv = list_entry(current_node_recv, pcb_t, list);
                unblock_pcb_recv -> status = TASK_READY;
                list_del(current_node_recv);
                list_add_tail(current_node_recv, &ready_queue);
            }
        }
        XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET, isr_reg);
        u32 rxsr_reg;
        rxsr_reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_RXSR_OFFSET);
        XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_RXSR_OFFSET, rxsr_reg | XEMACPS_RXSR_FRAMERX_MASK);  
        //local_flush_dcache_all();
        dmb();
        plic_irq_eoi(irq);
    }
    //send simulate time irq
    if(isr_reg & XEMACPS_IXR_TXCOMPL_MASK){
        //printk("Start Handling tx irq!\n");
        list_node_t *current_node_send;
        pcb_t *p;
        current_node_send = send_queue.next;
        if(!list_empty(&send_queue)){
            int received = bd_space[0x40] >> 63;
            printk("Received: %d\n",received);
            if(received){
                pcb_t *unblock_pcb;
                unblock_pcb = list_entry(current_node_send, pcb_t, list);
                unblock_pcb -> status = TASK_READY;
                list_del(current_node_send);
                list_add_tail(current_node_send, &ready_queue);
            }
        }
        XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_ISR_OFFSET, isr_reg);
        u32 txsr_reg;
        txsr_reg = XEmacPs_ReadReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET);
        XEmacPs_WriteReg(EmacPsInstance.Config.BaseAddress, XEMACPS_TXSR_OFFSET, txsr_reg | XEMACPS_TXSR_TXCOMPL_MASK);  
        //local_flush_dcache_all();
        dmb();
        plic_irq_eoi(irq);
    }
    // let PLIC know that handle_irq has been finished
}

//cause = c ----> This inst does not exit
void handle_inst_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    long cpu_id = get_current_cpu_id();
    uintptr_t lev0_kva;
    lev0_kva = check_ad(stval, pa2kva(current_running[cpu_id] ->pgdir));
    //If A & D not 1?
    if(lev0_kva != 0){
        set_attribute(lev0_kva, _PAGE_ACCESSED | _PAGE_DIRTY);
        local_flush_tlb_all();
        return;
    }else{
        prints(">Error: This Instruction does not Exit!");
        //Spin to refuse more errors
        while(1){
            ;
        }
    }
}

//cause = d ----> This memory space does not map
void handle_load_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
    long cpu_id = get_current_cpu_id();
    uintptr_t lev0_kva;
    lev0_kva = check_ad(stval, pa2kva(current_running[cpu_id] ->pgdir));
    //1. This page's A & D field is 0
    if(lev0_kva != 0){
        set_attribute(lev0_kva, _PAGE_ACCESSED);
        local_flush_tlb_all();
        return;
    }
    //2. This page's page has been swaped to disk(Clear va the last 12 bits)
    if(do_unswap(current_running[cpu_id], (stval >> 12) << 12)){
        //We limit the maximum pages a process can have to 4
        if(current_running[cpu_id] -> pnum >= 4){
            do_swap(current_running[cpu_id]);
        }
        local_flush_tlb_all();
        return;
    }
    //3. If the page_num excess our limits:
    if(current_running[cpu_id] -> pnum >= 4){
        do_swap(current_running[cpu_id]);
    }
    uintptr_t new_page;
    new_page = alloc_page_helper(stval, pa2kva(current_running[cpu_id]->pgdir), 1);
    current_running[cpu_id] -> pnum++;
    page_t *p = (page_t*)kmalloc(sizeof(page_t));
    p -> pa = kva2pa(new_page);
    p -> va = (stval >> 12) << 12;
    p -> atds = 0;
    list_add_tail(&(p -> list), &(current_running[cpu_id] -> plist));
    local_flush_tlb_all();
    
}

void handle_store_page_fault(regs_context_t *regs, uint64_t stval, uint64_t cause){
   long cpu_id = get_current_cpu_id();
    uintptr_t lev0_kva;
    lev0_kva = check_ad(stval, pa2kva(current_running[cpu_id] ->pgdir));
    //1. This page's A & D field is 0
    if(lev0_kva != 0){
        set_attribute(lev0_kva, _PAGE_ACCESSED | _PAGE_DIRTY);
        local_flush_tlb_all();
        return;
    }
    //2. This page's page has been swaped to disk(Clear va the last 12 bits)
    if(do_unswap(current_running[cpu_id], (stval >> 12) << 12)){
        //We limit the maximum pages a process can have to 4
        if(current_running[cpu_id] -> pnum >= 4){
            do_swap(current_running[cpu_id]);
        }
        local_flush_tlb_all();
        return;
    }
    //3. If the page_num excess our limits:
    if(current_running[cpu_id] -> pnum >= 4){
        do_swap(current_running[cpu_id]);
    }
    uintptr_t new_page;
    new_page = alloc_page_helper(stval, pa2kva(current_running[cpu_id]->pgdir), 1);
    current_running[cpu_id] -> pnum++;
    page_t *p = (page_t*)kmalloc(sizeof(page_t));
    p -> pa = kva2pa(new_page);
    p -> va = (stval >> 12) << 12;
    p -> atds = 0;
    list_add_tail(&(p -> list), &(current_running[cpu_id] -> plist));
    local_flush_tlb_all();
}

void init_exception()
{
    /* TODO: initialize irq_table and exc_table */
    /* note: handle_int, handle_syscall, handle_other, etc.*/
    int i;
    for(i = 0; i < IRQC_COUNT; i++){
        irq_table[i] = &handle_other;
    }
    irq_table[IRQC_S_TIMER] = &handle_int;      //Init handle_int
    //irq_table[IRQC_S_EXT]   =   &plic_handle_irq;   //Init Net_int
    for(i = 0; i < EXCC_COUNT; i++){
        exc_table[i] = &handle_other;
    }
    exc_table[EXCC_SYSCALL] = &handle_syscall;             //Init handle_syscall
    exc_table[EXCC_INST_PAGE_FAULT] = &handle_inst_page_fault;
    exc_table[EXCC_LOAD_PAGE_FAULT] = &handle_load_page_fault;
    exc_table[EXCC_STORE_PAGE_FAULT] = &handle_store_page_fault;
    setup_exception();
}

void handle_other(regs_context_t *regs, uint64_t stval, uint64_t cause)
{
    // Output more debug information
    char* reg_name[] = {
        "zero "," ra  "," sp  "," gp  "," tp  ",
        " t0  "," t1  "," t2  ","s0/fp"," s1  ",
        " a0  "," a1  "," a2  "," a3  "," a4  ",
        " a5  "," a6  "," a7  "," s2  "," s3  ",
        " s4  "," s5  "," s6  "," s7  "," s8  ",
        " s9  "," s10 "," s11 "," t3  "," t4  ",
        " t5  "," t6  "
    };
    for (int i = 0; i < 32; i += 3) {
        for (int j = 0; j < 3 && i + j < 32; ++j) {
            printk("%s : %016lx ",reg_name[i+j], regs->regs[i+j]);
        }
        printk("\n\r");
    }
    printk("sstatus: 0x%lx sbadaddr: 0x%lx scause: %lx\n\r",
           regs->sstatus, regs->sbadaddr, regs->scause);
    printk("stval: 0x%lx cause: %lx\n\r",
           stval, cause);
    printk("sepc: 0x%lx\n\r", regs->sepc);
    // printk("mhartid: 0x%lx\n\r", get_current_cpu_id());

    uintptr_t fp = regs->regs[8], sp = regs->regs[2];
    printk("[Backtrace]\n\r");
    printk("  addr: %lx sp: %lx fp: %lx\n\r", regs->regs[1] - 4, sp, fp);
    // while (fp < USER_STACK_ADDR && fp > USER_STACK_ADDR - PAGE_SIZE) {
    while (fp > 0x10000) {
        uintptr_t prev_ra = *(uintptr_t*)(fp-8);
        uintptr_t prev_fp = *(uintptr_t*)(fp-16);

        printk("  addr: %lx sp: %lx fp: %lx\n\r", prev_ra - 4, fp, prev_fp);

        fp = prev_fp;
    }

    assert(0);
}
