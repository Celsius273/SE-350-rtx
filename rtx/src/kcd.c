#include <LPC17xx.h>
#include <string.h>
#include "uart_polling.h"
#include "rtx.h"
#include "k_process.h"
#include "common.h"
#include "message_queue.h"
#include "kcd.h"

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
            k_send_message(cur->m_recv_pid, memcpy(k_request_memory_block(), message, 128));
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
void proc_kcd() {
	//init_kcd_process();
	int sender_id;
	while (1) {
		MSG_BUF* message = (MSG_BUF *)k_receive_message(&sender_id);


		if (message == NULL) {
			uart1_put_string("ERROR: The KCD received a null message!!!\n");
			continue;
		}

		if (message->mtype == KCD_REG) {
			kcd_process_command_registration(message);
		} else if (message->mtype == DEFAULT) {
			kcd_process_keyboard_input(message);
		} else {
			uart1_put_string("ERROR: The KCD received a message that was not of type command registration or keyboard input!\n");
		}
        
        k_release_memory_block(message);
	}
}
