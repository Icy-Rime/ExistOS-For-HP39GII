#include <stdint.h>
#include <stdio.h>


volatile uint32_t *PORT_TICK = (volatile uint32_t *)(0x02004000 + 0x7FF8);
volatile uint32_t *PORT_STDOUT = (volatile uint32_t *)(0x10000000);

//volatile uint32_t *PORT_TICK = (volatile uint32_t *)0xEFFF000c;
//volatile uint32_t *PORT_STDOUT = (volatile uint32_t *)0xe9000000;

extern unsigned int __bss_start;
extern unsigned int _end;

extern void (*__preinit_array_start[])(void) __attribute__((weak));
extern void (*__preinit_array_end[])(void) __attribute__((weak));
extern void (*__init_array_start[])(void) __attribute__((weak));
extern void (*__init_array_end[])(void) __attribute__((weak));

extern unsigned int _data_lma;
extern unsigned int _data;
extern unsigned int _edata;
 
  void _start()  __attribute__((naked)) __attribute__((section(".init"))) ;
  void _start() 
{ 
 
 

     __asm volatile("    \n\
     la sp,   _sp \n\
     la a0, _data_lma      \n\
     la a1, _data             \n\
     la a2, _edata        \n\
     bgeu a1, a2, 2f      \n\
 1:                                                   \n\
     lw t0, (a0)              \n\
     sw t0, (a1)          \n\
     addi a0, a0, 4    \n\
     addi a1, a1, 4       \n\
     bltu a1, a2, 1b      \n\
 2:                  " 
     );
 
     __asm volatile("    \n\
     la a0, __bss_start  \n\
     la a1, _end          \n\
     bgeu a0, a1, 4f      \n\
 3:                   \n\
     sw zero, (a0)        \n\
     addi a0, a0, 4       \n\
     bltu a0, a1, 3b      \n\
 4:   \n\
     j main    \n\
     ");

    //main();

}



int siprintf(char *buf, const char *fmt, ...);
char buf[512];

uint32_t get_tick_ms()
{
    return *PORT_TICK / 10000;
}

void delay(uint32_t ms)
{
    uint32_t start = get_tick_ms();
    while(get_tick_ms() - start < ms)
    {
        ;
    }
}

void puts_(char *s)
{
    while(*s)
    {
        *PORT_STDOUT = *s;
        s++;
    }
}
 

int main()
{
    int t = 0;
    while(1)
    {
        siprintf(buf, "test:%d\r\n", t);
        puts_(buf);
        t++;
        delay(1000);
    }
}

