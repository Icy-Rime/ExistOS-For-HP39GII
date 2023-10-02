


#include "board.h" 
#include "utils.h"

void bsp_wait_for_irq()
{
    __asm volatile (
    "mov R0, #0 \n"
    "mcr p15,0,r0,c7,c0,4 \n"
    "nop"
    );
}

uint32_t bsp_time_get_us(void)
{
    return HW_DIGCTL_MICROSECONDS_RD();
}

uint32_t bsp_time_get_ms(void)
{
    return HW_RTC_MILLISECONDS_RD();
}

void bsp_time_reset_mstick()
{
    INFO("Reset ms tick\r\n");
    HW_RTC_MILLISECONDS_CLR(BM_RTC_MILLISECONDS_COUNT);
}

uint32_t bsp_rtc_get_s(void)
{
    return HW_RTC_SECONDS_RD();
}

void bsp_rtc_set_s(uint32_t t)
{
    HW_RTC_SECONDS_WR(t);
}

void bsp_delayms(uint32_t ms)
{
    uint32_t start, cur;
    start = cur = HW_RTC_MILLISECONDS_RD();
    while (cur < start + ms) {
        cur = HW_RTC_MILLISECONDS_RD();
    }
}

void bsp_delayus(uint32_t us)
{
    uint32_t start, cur;
    start = cur = HW_DIGCTL_MICROSECONDS_RD();
    while (cur < start + us) {
        cur = HW_DIGCTL_MICROSECONDS_RD();
    }
}


void bsp_timer0_set(uint32_t us)
{
    if(us)
    {
        BF_CLRn(TIMROT_TIMCTRLn, 0, IRQ);
        bsp_enable_irq(HW_IRQ_TIMER0, true);
        BF_CS1n(TIMROT_TIMCOUNTn, 0, FIXED_COUNT, us/32);
        BF_CS1n(TIMROT_TIMCTRLn, 0, IRQ_EN, true);
        BF_CS1n(TIMROT_TIMCTRLn, 0, RELOAD, true);
        BF_CS1n(TIMROT_TIMCTRLn, 0, UPDATE, true);

    }else{
        bsp_enable_irq(HW_IRQ_TIMER0, false);
        BF_CS1n(TIMROT_TIMCTRLn, 0, IRQ_EN, false);
        BF_CS1n(TIMROT_TIMCTRLn, 0, RELOAD, false);
        BF_CS1n(TIMROT_TIMCTRLn, 0, UPDATE, false);
        BF_CS1n(TIMROT_TIMCOUNTn,0, FIXED_COUNT, 0);
        BF_CLRn(TIMROT_TIMCTRLn, 0, IRQ);
    }
}



void bsp_usb_phy_enable()
{
    BF_SET(CLKCTRL_PLLCTRL0, EN_USB_CLKS);
    HW_USBPHY_PWD_CLR(0xffffffff);
    BF_CLR(DIGCTL_CTRL, USB_CLKGATE);

}

void bsp_usb_dcd_int_enable(uint32_t enable)
{
    bsp_enable_irq(HW_IRQ_USB_CTRL, enable);
}

void bsp_reset()
{
    BF_WR(CLKCTRL_RESET, CHIP, 1);
}

void bsp_board_init()
{
    uint32_t uclkctrl;
    uclkctrl = HW_CLKCTRL_CPU_RD();
    uclkctrl |= BM_CLKCTRL_CPU_INTERRUPT_WAIT;
    HW_CLKCTRL_CPU_WR(uclkctrl);

}


void setHCLKDivider(uint32_t div) ;
void setCPUDivider(uint32_t div) ;

/*
0：	    CPU:   392MHz   HCLK:  196MHz
1：	    CPU:   196MHz   HCLK:  196MHz
2：	    CPU:   130MHz   HCLK:  130MHz
3：	    CPU:   98MHz    HCLK:  98MHz
4：	    CPU:   78MHz    HCLK:  78MHz
5：	    CPU:   65MHz    HCLK:  65MHz
6：	    CPU:   56MHz    HCLK:  56MHz
7：	    CPU:   49MHz    HCLK:  49MHz
8：	    CPU:   43MHz    HCLK:  43MHz
9：	    CPU:   39MHz    HCLK:  39MHz
10：	CPU:   35MHz    HCLK:  35MHz
11：	CPU:   32MHz    HCLK:  32MHz
12：	CPU:   30MHz    HCLK:  30MHz
13：	CPU:   28MHz    HCLK:  28MHz
14：	CPU:   26MHz    HCLK:  26MHz
15：	CPU:   24MHz    HCLK:  24MHz
*/
void bsp_set_perf_level(uint32_t n)
{
    if(n > 15)n=15;
    uint32_t d = n+1;
    if(n < 2)
    {
        setHCLKDivider(2); 
        setCPUDivider(d);
    }else{
        setCPUDivider(d);
        setHCLKDivider(1);
    }
    
}

uint32_t bsp_get_perf_level()
{
    return BF_RD(CLKCTRL_CPU, DIV_CPU) - 1;
}
