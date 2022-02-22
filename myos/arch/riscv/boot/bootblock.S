#include <asm/sbidef.h>
#include <asm/sbiasm.h>
#include <csr.h>
// 1. kernel address (move kernel to here ~) 
.equ kernel, 0x50201000
// 2. kernel main address (jmp here to start kernel main!) 
.equ kernel_main, 0x50301000
// 3. os size location (get the size of your OS here) 
.equ os_size_loc, 0x502001fc 
.text 
.global main  
main:
	// fence on all memory and I/O
	fence

    // a0 is mhartid, a1 is the pointer of fdt
	move s0,a0
	move s1,a1
    
    /* Load the global pointer */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
  
	// The core whose mhartid=0 is selected to boot the kernel.
	// Other cores should wait until the kernels initialization
	// has been finished.
    move a0, s0
	bnez a0, secondary

	// your code 
    la a0, msg
    SBI_CALL SBI_CONSOLE_PUTSTR 
    
    la s0, os_size_loc
    lh s0, 0(s0)
    li s1, 1
    li s2, 32768
    la s3, kernel
do_read_sector:
    mv a0, s3
    li a1, 64
    mv a2, s1
    SBI_CALL SBI_SD_READ
    addi s1, s1, 64
    add  s3, s3, s2
    addi s0, s0, -64
    bgtz s0, do_read_sector

    fence.i
 
    
    la t0,kernel_main
    jr t0
    
secondary:
	/* TODO:
	 * 1. Mask all interrupts
	 * 2. let stvec pointer to kernel_main
	 * 3. enable software interrupt for ipi
	 */ 
	fence
    
    // Mark all interrupt
    csrw CSR_SIE, zero

    // Set stvec to kernel entry
    la t0, kernel_main
    csrw CSR_STVEC, t0
    
    // Enable supervisor software interrupt (ipi)
    li t0, SIE_SSIE
    csrs CSR_SIE, t0
    
    li t0, SR_SIE
    csrw CSR_SSTATUS, t0
    

wait_for_wakeup:
	wfi
	j wait_for_wakeup 

stop:
	j stop
	nop

.data
msg : .string "It is a bootblock\n\r"

