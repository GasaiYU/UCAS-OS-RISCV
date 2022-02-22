#include <net.h>
#include <os/string.h>
#include <screen.h>
#include <emacps/xemacps_example.h>
#include <emacps/xemacps.h>

#include <os/sched.h>
#include <os/mm.h>



#define PER_RECV 32

EthernetFrame rx_buffers[RXBD_CNT];
EthernetFrame tx_buffer;
uint32_t rx_len[RXBD_CNT];

int net_poll_mode;

volatile int rx_curr = 0, rx_tail = 0;

long do_net_recv(uintptr_t addr, int* frLength, int num_packet, size_t size)
{
    // TODO: 
    // receive packet by calling network driver's function
    // wait until you receive enough packets(`num_packet`).
    // maybe you need to call drivers' receive function multiple times ?

    //We recv 32 pakets every time!
    int recv_packet = 0;           //Packet which has been received
    char* _addr = (char*)addr;     //Copy data
    long Status;
    int i;
    //printk("Kernel addr: %x\n",frLength);
    while(recv_packet < num_packet){
        if(num_packet - recv_packet <= PER_RECV){
            Status = EmacPsRecv(&EmacPsInstance, (EthernetFrame*)(rx_buffers), num_packet - recv_packet);
            if(net_poll_mode == 1){
                long cpu_id = get_current_cpu_id();
                pcb_t* recv_block_pcb;
                recv_block_pcb = list_entry(&(current_running[cpu_id] -> list), pcb_t, list);
                recv_block_pcb -> status = TASK_BLOCKED;
                list_add_tail(&(current_running[cpu_id] -> list), &recv_queue);
                do_scheduler();
            }
            Status = EmacPsWaitRecv(&EmacPsInstance, num_packet - recv_packet, rx_len);//(rx_len is Hardware Work)
            for(i = 0; i < num_packet - recv_packet; i++){
                memcpy(_addr, (char*)(&rx_buffers[i]), rx_len[i]);      //rx_len store bytes number
                _addr += rx_len[i];                                     //Addr+1 ---> +1 byte
                //printk("len[%d]: %d\n",i + recv_packet ,rx_len[i]);
                frLength[i + recv_packet] = rx_len[i];                  //Return bytes number
            }
        }else{
            Status = EmacPsRecv(&EmacPsInstance, (EthernetFrame*)(rx_buffers), PER_RECV);
            if(net_poll_mode == 1){
                long cpu_id = get_current_cpu_id();
                pcb_t* recv_block_pcb;
                recv_block_pcb = list_entry(&(current_running[cpu_id] -> list), pcb_t, list);
                recv_block_pcb -> status = TASK_BLOCKED;
                list_add_tail(&(current_running[cpu_id] -> list), &recv_queue);
                do_scheduler();
            }
            Status = EmacPsWaitRecv(&EmacPsInstance, PER_RECV, rx_len); //(rx_len is Hardware Work)
            for(i = 0; i < PER_RECV; i++){
                memcpy(_addr, (char*)(&rx_buffers[i]), rx_len[i]);      //rx_len store bytes number
                _addr += rx_len[i];                                     //Addr+1 ---> +1 byte
                frLength[i + recv_packet] = rx_len[i];              //Return bytes number
            }
        }
        recv_packet += PER_RECV;
    }
    //long ret = 0;
    return Status;
}

void do_net_send(uintptr_t addr, size_t length)
{
    // TODO:
    // send all packet
    // maybe you need to call drivers' send function multiple times --> every time we just send one packet
    //copy
    memcpy((char*)&tx_buffer, (char*)addr, length);
    //Send Packets. Note: Physical addr!
    EmacPsSend(&EmacPsInstance, (EthernetFrame*)kva2pa(&tx_buffer), length);
    if(net_poll_mode == 1){
        long cpu_id = get_current_cpu_id();
        pcb_t* send_block_pcb;
        send_block_pcb = list_entry(&(current_running[cpu_id] -> list), pcb_t, list);
        send_block_pcb -> status = TASK_BLOCKED;
        list_add_tail(&(current_running[cpu_id] -> list), &send_queue);
        do_scheduler();
    }
    EmacPsWaitSend(&EmacPsInstance);
}

void do_net_irq_mode(int mode)
{
    // TODO:
    // turn on/off network driver's interrupt mode
    net_poll_mode = mode;
}
