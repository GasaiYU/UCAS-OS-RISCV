#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>


int mthread_mutex_init(void* handle)
{
    /* TODO: */
    invoke_syscall(SYSCALL_MUTEX_LOCK_INIT,(int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;
}
int mthread_mutex_lock(void* handle) 
{
    /* TODO: */
    invoke_syscall(SYSCALL_MUTEX_LOCK_ACQUIRE, (int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;
}
int mthread_mutex_unlock(void* handle)
{
    /* TODO: */
    invoke_syscall(SYSCALL_MUTEX_LOCK_UNBLOCK, (int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;
}

int mthread_barrier_init(void* handle, unsigned count)
{   
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_INIT, (int*)handle, count, IGNORE, IGNORE);
    return 0;
}
int mthread_barrier_wait(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_BARRIER_WAIT, (int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;
}
int mthread_barrier_destroy(void* handle)
{
    invoke_syscall(SYSCALL_BARRIER_DESTROY, (int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;
    // TODO:
}

int mthread_semaphore_init(void* handle, int val)
{
    invoke_syscall(SYSCALL_SEM_INIT, (int*)handle, val, IGNORE, IGNORE);
    return 0;
    // TODO:
}
int mthread_semaphore_up(void* handle)
{   
    invoke_syscall(SYSCALL_SEM_UP, (int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;
    // TODO:
}
int mthread_semaphore_down(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEM_DOWN, (int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;    
}
int mthread_semaphore_destroy(void* handle)
{
    // TODO:
    invoke_syscall(SYSCALL_SEM_DESTROY, (int*)handle, IGNORE, IGNORE, IGNORE);
    return 0;
}

int mthread_join(mthread_t thread){
    invoke_syscall(SYSCALL_MTHREAD_JOIN, thread, IGNORE,IGNORE,IGNORE);
}

int mthread_create(mthread_t *thread, void (*start_routine)(void*), char *argv[]){
    int status = invoke_syscall(SYSCALL_MTHREAD_CREATE, thread, start_routine, argv, IGNORE);
    return status;
}