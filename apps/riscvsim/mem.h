#pragma once
#include <stdint.h>

#define MEMORY_BASE  (0x80000000)
#define MEMORY_SIZE  (64 * 1024)

#define ROM_BASE     (0x70000000) 


extern uint8_t *SRAM;
extern uint8_t *SROM;
extern uint32_t ROMSZ;


extern uint32_t timerVal;
extern uint32_t stdout_port;

#define TIMER_PORT  0xEFFF000c
#define STDOUT_PORT 0xe9000000

void update_perp();

#define fastprt(L1,ptrlen)  \
static inline ptrlen *fastptr##L1(uint32_t vaddr) \
{                                                           \
    uint32_t vaddr_seg = vaddr & 0xF0000000;                \
    uint32_t vaddr_off = vaddr & 0x0FFFFFFF;                \
    switch (vaddr_seg)                                      \
    {                                                       \
    case MEMORY_BASE:                                       \
         if(vaddr_off < MEMORY_SIZE)    \
            return  (ptrlen *)(SRAM + vaddr - MEMORY_BASE);       \
         else{printf("acc MEMORY_BASE om\r\n");return NULL;}   \
    case ROM_BASE:                                          \
         if(vaddr_off < ROMSZ)    \
         return  (ptrlen *)(SROM + vaddr - ROM_BASE);          \
         else{printf("acc ROM_BASE om\r\n");return NULL;}   \
    default:                                                    \
        update_perp();                                      \
        if(vaddr == TIMER_PORT)                                 \
        {                                                           \
            return (ptrlen *)&timerVal;                                \
        }else if (vaddr == STDOUT_PORT)                           \
        {                                                       \
            return (ptrlen *)&stdout_port;                       \
        }                                                       \
        printf("Access ERROR:%08lX\r\n", vaddr);               \
        return NULL;                                         \
    }                                                        \
}                                                            \

fastprt(8,uint8_t);
fastprt(16,uint16_t);
fastprt(32,uint32_t);


void dumpMem();
int memoryInit();

int memoryVirtAddrRead(uint32_t address, uint32_t sz,void *data);
int memoryVirtAddrWrite(uint32_t address, uint32_t sz, uint32_t data);



