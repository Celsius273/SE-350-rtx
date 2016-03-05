#include <LPC17xx.h>
#include "uart_polling.h"
#include "common.h"
#include "printf.h"
#include "crt.h"

void proc_crt() {
	while(1) {
		MSG_BUF* message = receive_message(NULL);
		
		if (NULL == message || message->mtype != CRT_DISPLAY) {
			printf("ERROR: The CRT received a message that is not type of CRT_DISPLAY");
		} else {
			printf("============ Running CRT Process =================");
			
			send_message(PID_UART_IPROC, message);
			LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef*) LPC_UART0;
			
			// We set the following bits to send a UART Interrupt, which 
			// jumps to the UART_Interrupt Handler, which runs the UART-i process
			// To print the message to the screen
			pUart->IER = IER_THRE | IER_RLS | IER_RBR;
			pUart->THR = '\0';
			
			// Release memory for the message
			release_memory_block(message);
		}
	}
}
