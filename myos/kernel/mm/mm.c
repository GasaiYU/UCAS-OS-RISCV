#include <os/list.h>
#include <os/mm.h>
#include <os/sched.h>
#include <pgtable.h>
#include <os/string.h>
#include <assert.h>
#define FREEKMALLOC 0xffffffc053000000

ptr_t memCurr = FREEMEM;
ptr_t kmallocCurr = FREEKMALLOC;

/*ptr_t allocPage(int numPage)
{
    // align PAGE_SIZE
    ptr_t ret = ROUND(memCurr, PAGE_SIZE);
    memCurr = ret + numPage * PAGE_SIZE;
    return ret;
}*/



static LIST_HEAD(freePageList);

/*We first find if there are available pages. If not, we alloc a new page*/
ptr_t allocPage()
{
    // TODO:
    if(!list_empty(&freePageList)){
        page_t *p = list_entry(freePageList.next, page_t, list);
        uintptr_t kva = pa2kva(p -> pa);
         for(int i = 0; i < 512; i++){
            *((uintptr_t*)kva + i) = 0;
        }
        list_del(freePageList.next);
        return kva;
    }else{
        //We do not return memCurr - 4096 because we previously set sp = allocPage(1)+4096
        memCurr += PAGE_SIZE;
        for(int j = 0; j < 512; j++){
            *((uintptr_t*)memCurr + j) = 0;
        }
        return memCurr;
    }
}

void freePage(ptr_t baseAddr)
{
    // TODO:
    page_t *fp = (page_t *)kmalloc(sizeof(page_t));
    fp->pa = baseAddr;
    //clear_pgdir(baseAddr);
    list_add_tail(&(fp->list), &freePageList);
}

//We use pre kmalloc. It is not a new function!
void *kmalloc(size_t size)
{
    // TODO(if you need):
    ptr_t ret = ROUND(kmallocCurr, 4);
    kmallocCurr = ret + size;
    return (void*)ret;
}

uintptr_t shm_page_get(int key)
{
    // TODO(c-core):
}

void shm_page_dt(uintptr_t addr)
{
    // TODO(c-core):
}

/* this is used for mapping kernel virtual address into user page table */
void share_pgtable(uintptr_t dest_pgdir, uintptr_t src_pgdir)
{
    // TODO:
}

/* allocate physical page for `va`, mapping it into `pgdir`,
   return the kernel virtual address for the page.
   */
//mode == 1 ---> USER, mode == 0 ---> not user
uintptr_t alloc_page_helper(uintptr_t va, uintptr_t pgdir, int mode)
{
    // TODO:
    //Get VPN
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
        //Set the valid and U
        if(mode == 1){
            set_attribute(lev2_addr, _PAGE_USER | _PAGE_PRESENT);
        }else{
            set_attribute(lev2_addr, _PAGE_PRESENT);
        }
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
        //Set the valid and U
        if(mode == 1){
            set_attribute(lev1_addr, _PAGE_USER | _PAGE_PRESENT);
        }else{
            set_attribute(lev1_addr, _PAGE_PRESENT);
        }
        //clear_pgdir((get_pa(*(lev1_addr))));
        lev0_addr = (PTE*)new_lev1_va + vpn0;     //We should use the va
    }else{
        ptr_t lev1_pa = (*lev1_addr >> 10) << 12;       //Get the pa
        uintptr_t lev1_va = pa2kva(lev1_pa);            //Transfer pa --> va
        lev0_addr = (PTE*)lev1_va + vpn0;                     //Use va to get lev1_addr
    }
    
    //Final ---> Get va. Get this idea from "boot.c"
    if(((*lev0_addr) & 0x1) != 0){
        return 0;       //We have already allocted Page
    }
    //We need to alloc a new page
    ptr_t new_lev0_va = allocPage();
    uintptr_t new_lev0_pa = kva2pa(new_lev0_va);
    set_pfn(lev0_addr, new_lev0_pa >> NORMAL_PAGE_SHIFT);
    //We don't set access and dirty 1
    if(mode == 1){
        set_attribute(lev0_addr, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC | _PAGE_USER /*| _PAGE_ACCESSED | _PAGE_DIRTY*/);
    }else{
        set_attribute(lev0_addr, _PAGE_PRESENT | _PAGE_READ | _PAGE_WRITE | _PAGE_EXEC /*| _PAGE_ACCESSED | _PAGE_DIRTY*/);
    }
    //clear_pgdir((get_pa(*(lev0_addr))));
    return new_lev0_va;    

}

uintptr_t alloc_page_helper_t(uintptr_t va, uintptr_t pgdir){
    uintptr_t return_addr;
    return_addr = alloc_page_helper(va, pgdir, 1);
    return return_addr;
}

//Reture lev0_page va
uintptr_t check_ad(uintptr_t va, uintptr_t pgdir){
    va &= VA_MASK;
    uint64_t vpn0 = (va >> NORMAL_PAGE_SHIFT) & ~(~0 << PPN_BITS);
    uint64_t vpn1 = (va >> NORMAL_PAGE_SHIFT + PPN_BITS) & ~(~0 << PPN_BITS);
    uint64_t vpn2 = (va >> NORMAL_PAGE_SHIFT + PPN_BITS + PPN_BITS) & ~(~0 << PPN_BITS);

    PTE *lev2_addr = (PTE*)pgdir + vpn2;
    PTE *lev1_addr = NULL;
    PTE *lev0_addr = NULL;

    if(((*lev2_addr) & 0x1) == 0){
        return 0;
    }
    lev1_addr = (PTE *)pa2kva(((*lev2_addr) >> 10) << 12) + vpn1;
    if(((*lev1_addr) & 0x1) == 0){
        return 0;
    }
    lev0_addr = (PTE *)pa2kva(((*lev1_addr) >> 10) << 12) + vpn0;
   if(((*lev0_addr) & 0x1) == 0){
       return 0;
   }
   return lev0_addr;        
}