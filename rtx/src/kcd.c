#include <string.h>
#include "common.h"
#include "message_queue.h"

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
    for (MSG_BUF *cur = entries; cur; cur = cur->mp_next) {
        if (strncmp(text, cur->mtext, strlen(cur->mtext)) == 0) {
            send_message(cur->m_recv_pid, memcpy(request_memory_block(), message, 128));
        }
    }
}

static MSG_BUF msg;
static int cmd_len = 0;
// Call uart_iproc_getc until it returns NO_CHAR
// Calls kcd_process_keyboard_input if appropriate
static void kcd_handle_keyboard_input(void) {
	for (int ch; (ch = uart_iproc_getc()) != NO_CHAR;) {
		switch (ch) {
			case '\n':
				break;
			case '\r':
				msg.mtext[cmd_len] = '\0';
				kcd_process_keyboard_input(&msg);
				cmd_len = 0;
				break;
			default:
				if (cmd_len < MTEXT_MAXLEN) {
					msg.mtext[cmd_len] = ch;
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
			uart1_put_string("ERROR: The KCD received a message that was not of type command registration or keyboard input!\n");
		}
		
    release_memory_block(message);
	}
}
