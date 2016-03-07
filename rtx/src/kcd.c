#include <string.h>
#include <assert.h>
#include "k_process.h"
#include "common.h"
#include "message_queue.h"
#include "printf.h"

#ifdef KCD_TEST

#include <stdio.h>
#include <stdlib.h>
#define uart1_put_string puts
#define uart_iproc_getc getchar

void send_message(int pid, MSG_BUF* msg) {
	printf("to pid %d: %s\n", pid, msg->mtext);
}

#else

#include <LPC17xx.h>
#include "uart_polling.h"
#include "uart.h"
#include "rtx.h"
#include "kcd.h"

// Read a char from the UART.
// NO_CHAR if no character is ready
extern int uart_iproc_getc(void);
#endif

MSG_BUF *entries = NULL;

static void kcd_process_command_registration(MSG_BUF* message) {
    // only put in queue don't process
    message->m_kdata[0] = 0;
    enqueue_message(message, &entries);
}

static void kcd_process_keyboard_input(MSG_BUF* message) {
	char *text = message->mtext;
	int sent_to_mask = 0;
	assert(NUM_PROCS <= 8 * sizeof(int));
	for (MSG_BUF *cur = entries; cur; cur = cur->mp_next) {
		if (strncmp(text, cur->mtext, strlen(cur->mtext)) == 0) {
			const int pid = cur->m_send_pid;
			const int pid_mask = 1 << pid;
			if (sent_to_mask & pid_mask) {
				continue;
			}
			sent_to_mask |= pid_mask;
			MSG_BUF *const block = request_memory_block();
			memcpy(block, message, 128);
			send_message(pid, block);
		}
	}
}

static char msg_buf[128];
static MSG_BUF *msg = (MSG_BUF *) msg_buf;
static int cmd_len = 0;
// Call uart_iproc_getc until it returns NO_CHAR
// Calls kcd_process_keyboard_input if appropriate
static void kcd_handle_keyboard_input(void) {
	for (int ch; (ch = uart_iproc_getc()) != NO_CHAR;) {
		switch (ch) {
			case '\n':
				break;
			case '\r':
				msg->mtext[cmd_len] = '\0';
				kcd_process_keyboard_input(msg);
				cmd_len = 0;
				break;
			case NO_CHAR:
				return;
			default:
				if (cmd_len < MTEXT_MAXLEN) {
					msg->mtext[cmd_len] = ch;
					++cmd_len;
				}
		}
	}
}

/*
command struct -> user types
KCD_REG: pid and char for command, 10 in array, something sends kcd with message type
create new command struct, make char field to command (copy it over) (message body text)
check which pid it's registered to, then send message to pid (etc etc)

all that's left is parsing the string

%W 

*/
void proc_kcd(void) {
	//init_kcd_process();
	int sender_id;
	for (;;) {
		MSG_BUF* message = (MSG_BUF *)receive_message(&sender_id);

		if (message == NULL) {
			uart1_put_string("ERROR: The KCD received a null message!!!\n");
			continue;
		}

		if (message->mtype == KCD_REG) {
			kcd_process_command_registration(message);
			continue; // don't free the block
		} else if (sender_id == PID_UART_IPROC) {
			kcd_handle_keyboard_input();
			continue; // don't free the block
		} else {
			printf("KCD got invalid message: message->mtype: %d sender_id: %d\n", message->mtype, sender_id);
		}
		
    release_memory_block(message);
	}
}
