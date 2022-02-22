#include <sbi.h>
#include <atomic.h>
#include <os/sched.h>
#include <os/smp.h>
#include <os/lock.h>



typedef struct spin_lock1
{
    volatile lock_status_t status;
} spin_lock_t1;

spin_lock_t1 kernel_lock;

void smp_init()
{
    /* TODO: */
    kernel_lock.status = UNLOCKED;
}

void wakeup_other_hart()
{
    /* TODO: */
    sbi_send_ipi(NULL);     //Send IPI to All Cores
    
}

void lock_kernel()
{
    /* TODO: */
    //If the kernel is locked --> SPIN
    while(atomic_cmpxchg(UNLOCKED, LOCKED, (ptr_t)&kernel_lock.status)){
        ;
    }
}

void unlock_kernel()
{
    /* TODO: */
    kernel_lock.status = UNLOCKED;
}

