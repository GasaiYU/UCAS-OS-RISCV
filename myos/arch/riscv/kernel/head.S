/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2012 Regents of the University of California
 */

#include <asm.h>
#include <csr.h>

/* maybe you need another stack for booting? */
.equ boot_stack_top_base, 0x50202000

.section ".entry_function","ax"
ENTRY(_start)
  /* Mask all interrupts */
  csrw CSR_SIE, zero
  csrw CSR_SIP, zero

  /*
   * Disable FPU to detect illegal usage of
   * floating point in kernel space
   */
  li t0, SR_FS
  csrc sstatus, t0

  /* Load the global pointer */
  .option push
  .option norelax
  la gp, __global_pointer$
  .option pop

  la sp, boot_stack_top_base
  
  csrr a0, CSR_MHARTID      //Get the CPU ID

  /*this should never return*/
  call boot_kernel          //Jump to boot_kernel

loop:
  wfi
  j loop

END(_start)
