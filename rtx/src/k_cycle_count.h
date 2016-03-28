#ifndef K_CYCLE_COUNT_H_
#define K_CYCLE_COUNT_H_

void k_cycle_count_init(void);

// Systick regs
#define STCSR ((volatile unsigned *)0xE000E010)
#define STRVR ((volatile unsigned *)0xE000E014)
#define STCVR ((volatile unsigned *)0xE000E018)
#define get_cycle_count24() (*STCVR)

unsigned cycle_count_difference(unsigned STCVR1, unsigned STCVR2);

// Taken from NXP/LPC17xx/system_LPC17xx.c
// Copyright (C) 2009-2012 ARM Limited. All rights reserved.

#define CLOCK_SETUP           1
#define SCS_Val               0x00000020
#define CLKSRCSEL_Val         0x00000001
#define PLL0_SETUP            1
#define PLL0CFG_Val           0x00050063
#define PLL1_SETUP            1
#define PLL1CFG_Val           0x00000023
#define CCLKCFG_Val           0x00000003
#define USBCLKCFG_Val         0x00000000
#define PCLKSEL0_Val          0x00000000
#define PCLKSEL1_Val          0x00000000
#define PCONP_Val             0x042887DE
#define CLKOUTCFG_Val         0x00000000

/*----------------------------------------------------------------------------
  Define clocks
 *----------------------------------------------------------------------------*/
#define XTAL        (12000000UL)        /* Oscillator frequency               */
#define OSC_CLK     (      XTAL)        /* Main oscillator frequency          */
#define RTC_CLK     (   32768UL)        /* RTC oscillator frequency           */
#define IRC_OSC     ( 4000000UL)        /* Internal RC oscillator frequency   */


/* F_cco0 = (2 * M * F_in) / N  */
#define __M               (((PLL0CFG_Val      ) & 0x7FFF) + 1)
#define __N               (((PLL0CFG_Val >> 16) & 0x00FF) + 1)
#define __FCCO(__F_IN)    ((2ULL * __M * __F_IN) / __N)
#define __CCLK_DIV        (((CCLKCFG_Val      ) & 0x00FF) + 1)

/* Determine core clock frequency according to settings */
 #if (PLL0_SETUP)
    #if   ((CLKSRCSEL_Val & 0x03) == 1)
        #define __CORE_CLK (__FCCO(OSC_CLK) / __CCLK_DIV)
    #elif ((CLKSRCSEL_Val & 0x03) == 2)
        #define __CORE_CLK (__FCCO(RTC_CLK) / __CCLK_DIV)
    #else
        #define __CORE_CLK (__FCCO(IRC_OSC) / __CCLK_DIV)
    #endif
 #else
    #if   ((CLKSRCSEL_Val & 0x03) == 1)
        #define __CORE_CLK (OSC_CLK         / __CCLK_DIV)
    #elif ((CLKSRCSEL_Val & 0x03) == 2)
        #define __CORE_CLK (RTC_CLK         / __CCLK_DIV)
    #else
        #define __CORE_CLK (IRC_OSC         / __CCLK_DIV)
    #endif
 #endif

#endif /* K_CYCLE_COUNT_H_ */