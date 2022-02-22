#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <type.h>



uint64_t time_elapsed = 0;
uint32_t time_base = 0;

//The cycles from starting to now
uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

//The total second
uint64_t get_timer()
{
    return get_ticks() / time_base;
}

//Cycles per second
uint64_t get_time_base()
{
    return time_base;
}

//latence time cycles
void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

//add code
uint32_t get_wall_time(uint32_t *time_elapsed){
  *time_elapsed = get_ticks();
  uint32_t time = get_time_base();
  return time;
}
