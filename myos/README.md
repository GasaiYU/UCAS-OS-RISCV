# RISC-V OS

#### 1. File struture:

##### 	|-arch/riscv:

​     	**|-boot**

​		  	bootblock.S: BootLoader. Load the kernel into memory.

​		 **|-include**: Some header files contain some macros

​		 **|-kernel:** 

​		    boot.c: kernel space map

​			entry.S: save and restore contex here

​			start.S: set C environment

​			trap.S: Save exception_handler_entry and enable global exceptions

​		**|-sbi:** SBI invoke.

​		crt0.S: load gp, clear bss, call main and exit.

​	**|-drivers:**

​		**emacps: **Mac network driver

​		net.c, net.h: Invoke driver to realize send and receive packets.

​		plic.c, plic.h: Set network driver interrupt

​		screen.c, screen,h: Handle print on screen

​	**|-include: **Some header files contain some macros

​	**|-init:**

​		main.c:  The entry of the kernel!

​	**|-kernel:**

​		**|-fs:**

​			fs.c: File System

​		**|-irq:**

​			irq.c: handle exceptions

​		**|-locking:**

​			lock.c: Some synchronize primitives like lock and barrier.

​		**|-mm:**

​			ioremap.c: map io space

​			mm.c: VM support

​		**|-sched:**

​			sched.c: Task schedule

​			smp.c: Lock one core when two cores want to get into the kernel at the same time

​			time.c: get clock ticks

​		**|-syscall:**

​			syscall.c: Direct syscall to its certain function

**|-libs:** User mode libs

**|-test: **test programs and shell

**|-tiny_libc: **Kernel mode libs

**|-tools:** Some useful auxiliary means.			

#### 2. Memory Layout

```
     Phy addr    |   Memory    |     Kernel vaddr

        N/A      +-------------+  0xffffffe002000000
                 | I/O  Mapped | 
        N/A      +-------------+  0xffffffe000000000
                 |     ...     | 
    0x5f000000   +-------------+  0xffffffc05f000000
                 | Kernel PTEs | 
    0x5e000000   +-------------+  0xffffffc05e000000
                 | Kernel Heap | 
    0x5d000000   +-------------+  0xffffffc05d000000
                 |   Free Mem  |
                 |     Pages   | 
                 |  for alloc  | 
    0x51000000   +-------------+  0xffffffc051000000
                 |     ...     | 
    0x50500000   +-------------+  0xffffffc050500000
                 |   Kernel    |
                 |   Segments  |
    0x50400000   +-------------+  0xffffffc050400000
                 | vBoot Setup |
                 |   (boot.c)  |
    0x50300000   +-------------+  0xffffffc050300000
                 |     ...     |
    0x50201000   +-------------+  N/A
                 |  Bootblock  |
    0x50200000   +-------------+  N/A
```

#### 3. Usage

​	Compile and run

- For compiling all, please execute `make all` in the top directory.
- For write image into SD-card, please execute `make floppy` and remember to modify `DISK` macro in `Makefile` to your own SD-card symbol.
- For running in QEMU, please execute `make qemu` and remember to change the `QEMU_PATH` option in `Makefile`.
- For running in QEMU and debugs with gdb, please execute `make qemu-gdb` and remember to change the `QEMU_PATH` option in `Makefile`.

