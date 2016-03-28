#include "k_cycle_count.h"

// Can Cortex-M3 measure the cycle count of its own activity?
// http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.faqs/ka8713.html

// Can Cortex-M3 measure the cycle count of its own activity?
// Answer

// Cortex-M3 can use its SysTick function to measure elapsed cycle counts.

// The SysTick function must be configured to use the processor clock as the reference timing source. The count will be accurate up to a 24-bit maximum number of clock cycles from the point where the STCVR is re-loaded.

void k_cycle_count_init(void) {
#ifdef USE_SYSTICK
	// Configure Systick   
	*STRVR = 0xFFFFFF;  // max count
	*STCVR = 0;         // force a re-load of the counter value register
	*STCSR = 5;         // enable FCLK count without interrupt
#else
	LPC_SC->PLL1CFG = 0x11;
	LPC_SC->PCONP |= 1<<2;
	LPC_SC->PCLKSEL0 |= 0x01 << 4;
	LPC_TIM1->PR = 0;
	LPC_TIM1->PC = 0;
	LPC_TIM1->TC = 0;
	LPC_TIM1->TCR |= 1;
#endif
}

unsigned cycle_count_difference(unsigned start, unsigned end) {
#ifdef USE_SYSTICK
	unsigned delta = start - end;
	unsigned mask = 0x00ffffff;
#else
	unsigned delta = end - start;
	unsigned mask = 0xffffffff;
#endif
	// The cycle count for an operation can then be obtained by reading the STCVR immediately before and immediately after the operation in question. As STCVR is a down counter, the number of core clock cycles taken by the operation is given by:
	unsigned difference = ((delta - 2) & mask);
	if (difference == mask) {
		difference = 0;
	}
	return difference;
	// The overhead of 2 cycles is because the read of the STCVR is Strongly Ordered with regard to other memory accesses or data processing instructions, although there is an exception to this general rule; two consecutive reads to the STCVR, without any intervening data processing or external memory access instrucitons, will pipeline and therefore show an overhead of only 1 cycle (ie. the STCVR values will show a difference of only 1).
}