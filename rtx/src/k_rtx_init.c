/** 
 * @file:   k_rtx_init.c
 * @brief:  Kernel initialization C file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */

#include "k_rtx_init.h"
#include "uart_polling.h"
#include "uart.h"
#include "k_memory.h"
#include "k_process.h"
#include "k_cycle_count.h"
#include "timer.h"

#include "allow_k.h"

void k_rtx_init(void)
{
	disable_irq();
	k_cycle_count_init();
	uart_irq_init(0);   // uart0, interrupt-driven 
	uart1_init();       // uart1, polling
	timer_init(0);
	memory_init();
	process_init();
	enable_irq();
	
	uart1_put_string("RTX is starting\n\r");
	/* start the first process */
	k_release_processor();
}
