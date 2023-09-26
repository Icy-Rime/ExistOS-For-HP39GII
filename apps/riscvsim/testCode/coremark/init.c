
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



}

