#include <os/ioremap.h>
#include <os/mm.h>
#include <pgtable.h>
#include <type.h>

// maybe you can map it to IO_ADDR_START ?
static uintptr_t io_base = IO_ADDR_START;

void *ioremap(unsigned long phys_addr, unsigned long size)
{
    // map phys_addr to a virtual address
    // then return the virtual address
    //This is the pgdir va
    //We copy codes in mm.c in prj4
    uintptr_t pgdir = 0xffffffc05e000000;
    int init_status = 0;
    uintptr_t* vaddr = NULL;
    while(size > 0){
        //Every io's va = VA_BASE_ADDR
        uint64_t va = io_base;

        va &= VA_MASK;
        uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & ~(~0 << PPN_BITS);
        uint64_t vpn1 = (va >> NORMAL_PAGE_SHIFT + PPN_BITS) & ~(~0 << PPN_BITS);
        uint64_t vpn2 = (va >> NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS) & ~(~0 << PPN_BITS);

        PTE *lev2_addr = (PTE*)pgdir + vpn2;
        PTE *lev1_addr = NULL;
        PTE *lev0_addr = NULL;

           //Invalid ---> Lev2
        if(((*lev2_addr) & 0x1) == 0){
        //First, we get the va of the new page
            ptr_t new_lev2_va = allocPage();
            uintptr_t new_lev2_pa = kva2pa(new_lev2_va);
            //Map
            //set_pfn(lev2_addr, new_lev2_pa >> NORMAL_PAGE_SHIFT);
            *lev2_addr = (kva2pa(new_lev2_va) >> 12) << 10;
            //Set the valid
            set_attribute(lev2_addr, _PAGE_PRESENT);
            //clear_pgdir((get_pa(*(lev2_addr))));
            lev1_addr = (PTE*)new_lev2_va + vpn1;     //We should use the va
        }else{
            ptr_t lev2_pa = (*lev2_addr >> 10) << 12;       //Get the pa
            uintptr_t lev2_va = pa2kva(lev2_pa);            //Transfer pa --> va
            lev1_addr = (PTE*)lev2_va + vpn1;               //Use va to get lev1_addr
        }

            //Invalid ---> level 1
        if(((*lev1_addr) & 0x1) == 0){
            //First, we get the va of the new page
            ptr_t new_lev1_va = allocPage();
            uintptr_t new_lev1_pa = kva2pa(new_lev1_va);
            //Map
            *lev1_addr = (kva2pa(new_lev1_va) >> 12) << 10;
            //Set the valid
            set_attribute(lev1_addr, _PAGE_PRESENT);
            //clear_pgdir((get_pa(*(lev1_addr))));
            lev0_addr = (PTE*)new_lev1_va + vpn0;     //We should use the va
        }else{
            ptr_t lev1_pa = (*lev1_addr >> 10) << 12;       //Get the pa
            uintptr_t lev1_va = pa2kva(lev1_pa);            //Transfer pa --> va
            lev0_addr = (PTE*)lev1_va + vpn0;                     //Use va to get lev1_addr
        }

        set_pfn(lev0_addr, phys_addr >> NORMAL_PAGE_SHIFT);
        set_attribute(lev0_addr, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_ACCESSED | _PAGE_DIRTY);
        //We return these pages' lowest addr
        if(init_status == 0){
            init_status = 1;
            vaddr = io_base;
        }
        //We alloc a page, io_base is static
        io_base += PAGE_SIZE;
        phys_addr += PAGE_SIZE;
        size -= PAGE_SIZE;
    }
    //FLush TLB
    local_flush_tlb_all();
    return vaddr; 
}

void iounmap(void *io_addr)
{
    // TODO: a very naive iounmap() is OK
    // maybe no one would call this function?
    // *pte = 0;
}
