#include <mailbox.h>
#include <string.h>
#include <sys/syscall.h>

mailbox_t mbox_open(char *name)
{
    // TODO:
    mailbox_t mailbox_id;
    mailbox_id = invoke_syscall(SYSCALL_MBOX_OPEN, (uintptr_t)name, IGNORE, IGNORE,IGNORE);
    return mailbox_id;
}

void mbox_close(mailbox_t mailbox)
{
    // TODO:
    invoke_syscall(SYSCALL_MBOX_CLOSE, mailbox, IGNORE, IGNORE, IGNORE);
}

int mbox_send(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO:
    int block_num;
    block_num = invoke_syscall(SYSCALL_MBOX_SEND, mailbox, (uintptr_t)msg, msg_length, IGNORE);
    return block_num;
}

int mbox_recv(mailbox_t mailbox, void *msg, int msg_length)
{
    // TODO: 
    int block_num;
    block_num = invoke_syscall(SYSCALL_MBOX_RECV, mailbox, (uintptr_t)msg, msg_length, IGNORE);
    return block_num;
}
