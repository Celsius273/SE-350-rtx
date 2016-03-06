/**
 * @brief: uart_irq.c 
 * @author: NXP Semiconductors
 * @author: Y. Huang
 * @date: 2014/02/28
 */

#include <LPC17xx.h>
#include "uart.h"
#include "k_rtx.h"
#include "uart_polling.h"
#include "list.h"
#ifdef DEBUG_0
#include "printf.h"
#endif
#include "k_process.h"
#include "allow_k.h"

#define UART(i) ((LPC_UART_TypeDef *)LPC_UART ## i)

// Whether the uart transmit holding register being transmitted
volatile bool uart_thre = false;
volatile bool uart_iproc_notif_in = true;
volatile bool uart_iproc_notif_out = true;
LL_DECLARE(volatile outbuf, uint8_t, 200);
#define OUTBUF_THRESHOLD 100
LL_DECLARE(volatile inbuf, uint8_t, 200);
MSG_BUF notif_in_msg;
MSG_BUF notif_out_msg;

static bool check_hotkey(uint8_t ch) {
#ifdef _DEBUG_HOTKEYS
	switch (ch) {
		case HOTKEY_READY_QUEUE:
			k_print_ready_queue();
			return true;
		case HOTKEY_BLOCKED_MEM_QUEUE:
			k_print_blocked_on_memory_queue();
			return true;
		case HOTKEY_BLOCKED_MSG_QUEUE:
			k_print_blocked_on_receive_queue();
			return true;
		case HOTKEY_MSG_LOG:
			k_print_message_log();
			break;
	}
#endif
	return false;
}

// Send the input character to the appropriate process(es)
static void uart_send_input_char(uint8_t ch) {
	if (check_hotkey(ch)) {
		return;
	}
	if (LL_SIZE(inbuf) == LL_CAPACITY(inbuf)) {
		// Drop the character
		return;
	}
	LL_PUSH_BACK(inbuf, ch);
	if (ch == '\r') {
		if (uart_iproc_notif_in) {
			uart_iproc_notif_in = false;
			notif_in_msg.mtype = DEFAULT;
			k_send_message_helper(PID_UART_IPROC, PID_KCD, &notif_in_msg);
		}
	}
}

// Return the next character to output, or NO_CHAR
static int uart_pop_output_char(void) {
	if (LL_SIZE(outbuf) == 0) {
		if (uart_iproc_notif_out) {
			uart_iproc_notif_out = false;
			notif_in_msg.mtype = DEFAULT;
			k_send_message_helper(PID_UART_IPROC, PID_CRT, &notif_out_msg);
		}
		return NO_CHAR;
	}
	return LL_POP_FRONT(outbuf);
}

// Queue a character for printing, without blocking
static void uart_putc_nonblocking(uint8_t ch) {
	if (LL_SIZE(outbuf) != LL_CAPACITY(outbuf)) {
		LL_PUSH_BACK(outbuf, ch);
	}
	// LL_SIZE(outbuf) != 0
	if (!uart_thre) {
		// Ensure the THRE interrupt is enabled
		ch = LL_POP_FRONT(outbuf);
		uart_thre = true;
		LPC_UART_TypeDef *pUart = UART(0);
		pUart->THR = ch;
		pUart->IER |= IER_THRE; // Interrupt Enable Register: Transmit Holding Register Empty
	}
}

// Public UART APIs

// Read a character, or NO_CHAR
// Enables input notification if NO_CHAR
int uart_iproc_getc(void) {
	int ret;
	disable_irq();
	if (LL_SIZE(inbuf) == 0) {
		uart_iproc_notif_in = true;
		ret = NO_CHAR;
	} else {
		ret = LL_POP_FRONT(inbuf);
	}
	enable_irq();
	return ret;
}

// Write a character, or return 0 if failed
bool uart_iproc_putc(uint8_t ch) {
	bool ret;
	disable_irq();
	if (LL_SIZE(outbuf) > OUTBUF_THRESHOLD) {
		uart_iproc_notif_out = true;
		ret = false;
	} else {
		uart_putc_nonblocking(ch);
		ret = true;
	}
	enable_irq();
	return ret;
}

// UART low-level handlers

/**
 * @brief: initialize the n_uart
 * NOTES: It only supports UART0. It can be easily extended to support UART1 IRQ.
 * The step number in the comments matches the item number in Section 14.1 on pg 298
 * of LPC17xx_UM
 */
int uart_irq_init(int n_uart) {

	LPC_UART_TypeDef *pUart;

	if ( n_uart ==0 ) {
		/*
		Steps 1 & 2: system control configuration.
		Under CMSIS, system_LPC17xx.c does these two steps
		 
		-----------------------------------------------------
		Step 1: Power control configuration. 
		        See table 46 pg63 in LPC17xx_UM
		-----------------------------------------------------
		Enable UART0 power, this is the default setting
		done in system_LPC17xx.c under CMSIS.
		Enclose the code for your refrence
		//LPC_SC->PCONP |= BIT(3);
	
		-----------------------------------------------------
		Step2: Select the clock source. 
		       Default PCLK=CCLK/4 , where CCLK = 100MHZ.
		       See tables 40 & 42 on pg56-57 in LPC17xx_UM.
		-----------------------------------------------------
		Check the PLL0 configuration to see how XTAL=12.0MHZ 
		gets to CCLK=100MHZin system_LPC17xx.c file.
		PCLK = CCLK/4, default setting after reset.
		Enclose the code for your reference
		//LPC_SC->PCLKSEL0 &= ~(BIT(7)|BIT(6));	
			
		-----------------------------------------------------
		Step 5: Pin Ctrl Block configuration for TXD and RXD
		        See Table 79 on pg108 in LPC17xx_UM.
		-----------------------------------------------------
		Note this is done before Steps3-4 for coding purpose.
		*/
		
		/* Pin P0.2 used as TXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 4);  
		
		/* Pin P0.3 used as RXD0 (Com0) */
		LPC_PINCON->PINSEL0 |= (1 << 6);  

		pUart = (LPC_UART_TypeDef *) LPC_UART0;	 
		
	} else if ( n_uart == 1) {
	    
		/* see Table 79 on pg108 in LPC17xx_UM */ 
		/* Pin P2.0 used as TXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 0);

		/* Pin P2.1 used as RXD1 (Com1) */
		LPC_PINCON->PINSEL4 |= (2 << 2);	      

		pUart = (LPC_UART_TypeDef *) LPC_UART1;
		
	} else {
		return 1; /* not supported yet */
	} 
	
	/*
	-----------------------------------------------------
	Step 3: Transmission Configuration.
	        See section 14.4.12.1 pg313-315 in LPC17xx_UM 
	        for baud rate calculation.
	-----------------------------------------------------
        */
	
	/* Step 3a: DLAB=1, 8N1 */
	pUart->LCR = UART_8N1; /* see uart.h file */ 

	/* Step 3b: 115200 baud rate @ 25.0 MHZ PCLK */
	pUart->DLM = 0; /* see table 274, pg302 in LPC17xx_UM */
	pUart->DLL = 9;	/* see table 273, pg302 in LPC17xx_UM */
	
	/* FR = 1.507 ~ 1/2, DivAddVal = 1, MulVal = 2
	   FR = 1.507 = 25MHZ/(16*9*115200)
	   see table 285 on pg312 in LPC_17xxUM
	*/
	pUart->FDR = 0x21;       
	
 

	/*
	----------------------------------------------------- 
	Step 4: FIFO setup.
	       see table 278 on pg305 in LPC17xx_UM
	-----------------------------------------------------
        enable Rx and Tx FIFOs, clear Rx and Tx FIFOs
	Trigger level 0 (1 char per interrupt)
	*/
	
	pUart->FCR = 0x07;

	/* Step 5 was done between step 2 and step 4 a few lines above */

	/*
	----------------------------------------------------- 
	Step 6 Interrupt setting and enabling
	-----------------------------------------------------
	*/
	/* Step 6a: 
	   Enable interrupt bit(s) wihtin the specific peripheral register.
           Interrupt Sources Setting: RBR, THRE or RX Line Stats
	   See Table 50 on pg73 in LPC17xx_UM for all possible UART0 interrupt sources
	   See Table 275 on pg 302 in LPC17xx_UM for IER setting 
	*/
	/* disable the Divisior Latch Access Bit DLAB=0 */
	pUart->LCR &= ~(BIT(7)); 
	
	//pUart->IER = IER_RBR | IER_THRE | IER_RLS; 
	pUart->IER = IER_RBR | IER_RLS;

	/* Step 6b: enable the UART interrupt from the system level */
	
	if ( n_uart == 0 ) {
		NVIC_EnableIRQ(UART0_IRQn); /* CMSIS function */
	} else if ( n_uart == 1 ) {
		NVIC_EnableIRQ(UART1_IRQn); /* CMSIS function */
	} else {
		return 1; /* not supported yet */
	}
	pUart->THR = '\0';
	return 0;
}

/**
 * @brief: use CMSIS ISR for UART0 IRQ Handler
 * NOTE: This example shows how to save/restore all registers rather than just
 *       those backed up by the exception stack frame. We add extra
 *       push and pop instructions in the assembly routine. 
 *       The actual c_UART0_IRQHandler does the rest of irq handling
 */
__asm void UART0_IRQHandler(void)
{
	PRESERVE8
	IMPORT c_UART0_IRQHandler
	IMPORT k_check_preemption
	PUSH{r4-r11, lr}
	BL c_UART0_IRQHandler
	POP{r4-r11, pc}
} 
/**
 * @brief: c UART0 IRQ Handler
 */
void c_UART0_IRQHandler(void)
{
	disable_irq();
	uint8_t IIR_IntId;	    // Interrupt ID from IIR 		 
	LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *)LPC_UART0;
	
#ifdef DEBUG_0
	uart1_put_string("Entering c_UART0_IRQHandler\n\r");
#endif // DEBUG_0

	/* Reading IIR automatically acknowledges the interrupt */
	IIR_IntId = (pUart->IIR) >> 1 ; // skip pending bit in IIR 
	if (IIR_IntId & IIR_RDA) { // Receive Data Avaialbe
		/* read UART. Read RBR will clear the interrupt */
		uint8_t ch = pUart->RBR;
		if (ch != 127 && ch != '\b') { // skip backspace
			// Echo-back before it's processed
			uart_putc_nonblocking(ch);
			if (ch == '\r') {
				uart_putc_nonblocking('\n');
			}
			uart_send_input_char(ch);
		}
	} else if (IIR_IntId & IIR_THRE) {
	/* THRE Interrupt, transmit holding register becomes empty */

		int ch = uart_pop_output_char();
		if (ch == NO_CHAR) {
			// Disable the interrupt
			UART(0)->IER &= ~IER_THRE;
			uart_thre = false;
		} else {
			UART(0)->THR = ch;
		}
	} else {  /* not implemented yet */
#ifdef DEBUG_0
			uart1_put_string("Should not get here!\n\r");
#endif // DEBUG_0
	}	
	enable_irq();
	k_check_preemption();
}
