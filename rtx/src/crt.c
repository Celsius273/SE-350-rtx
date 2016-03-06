#include "uart.h"
#include "crt.h"
#include "message_queue.h"

MSG_BUF *output = NULL;
int offset = 0;

void proc_crt(void) {
	for (;;) {
		int from = -1;
		MSG_BUF* msg = receive_message(&from);
		if (msg == NULL) {
			assert(0);
			continue;
		} else if (msg->mtype == CRT_DISPLAY) {
			msg->m_kdata[0] = 0;
			enqueue_message(msg, &output);
			// We're unblocked.
		} else if (from == PID_UART_IPROC) {
			// We're unblocked.
		} else {
			assert(0);
		}

		// Block ourselves
		while (!is_queue_empty(&output)) {
			// Check if we have a character to print.
			// Allow printing the last character, if it's not '\0'
			if (!(offset <= MTEXT_MAXLEN && output->mtext[offset])) {
				MSG_BUF *empty = output;
				output = output->mp_next;
				offset = 0;
				release_memory_block(empty);
				continue;
			}

			// Try to print the next character
			if (!uart_iproc_putc(output->mtext[offset])) {
				// Blocked!
				break;
			}
			++offset;
		}
	}
}
