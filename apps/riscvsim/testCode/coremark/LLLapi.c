
#include "LLLapi.h"

volatile uint32_t *PORT_TICK = (volatile uint32_t *)0xEFFF000c;
volatile uint32_t *PORT_STDOUT = (volatile uint32_t *)0xe9000000;


//volatile uint32_t *PORT_TICK = (volatile uint32_t *)(0x02004000 + 0x7FF8);
//volatile uint32_t *PORT_STDOUT = (volatile uint32_t *)(0x10000000);


uint32_t get_tick_ms()
{
    return *PORT_TICK ;/// 10000;
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

void ll_putc(int c)
{
    *PORT_STDOUT = c;
}
