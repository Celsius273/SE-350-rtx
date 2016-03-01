#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "k_memory.h"
#include "usr_clock.h"

// Wall clock display

#define MTEXT_MAXLEN (sizeof(mem_t) - offsetof(struct msgbuf, mtext) - 1)

static void kcd_register(const char *cmd_prefix)
{
	assert(strlen(cmd_prefix) <= MTEXT_MAXLEN);

	struct msgbuf *p_msg_env = request_memory_block();
	p_msg_env->mtype = KCD_REG;
	strcpy(p_msg_env->mtext, cmd_prefix);
	send_message(PID_KCD, p_msg_env);
	p_msg_env = NULL;
}

static int crt_printf(const char *fmt, ...) {
	struct msgbuf *msg = request_memory_block();
	int ret;
	{
		va_list va;
		va_start(va, fmt);
		ret = vsnprintf(msg->mtext, sizeof(msg->mtext), fmt, va);
		va_end(va);
	}
	send_message(PID_CRT, msg);
	return ret;
}

static volatile int clock_tick = 0;
static volatile unsigned int clock_h, clock_m, clock_s;

/**
 * Print the time and schedule another tick.
 * Optionally take a message with the expected clock tick.
 */
static void clock_handle_tick(struct msgbuf *msg)
{
	if (msg) {
		int clock_tick_ = clock_tick;
		if (memcmp(msg->mtext, &clock_tick_, sizeof(clock_tick_))) {
			return;
		}
		if ((clock_s = (clock_s + 1) % 60) == 0) {
			if ((clock_m = (clock_m + 1) % 60) == 0) {
				if ((clock_h = (clock_h + 1) % 60) == 0) {
					// next day, do nothing
				}
			}
		}
	} else {
		msg = request_memory_block();
	}

	crt_printf("Wall clock: %02u:%02u:%02u\n", clock_h, clock_m, clock_s);

	{
		int clock_tick_ = clock_tick;
		memcpy(msg->mtext, &clock_tick_, sizeof(clock_tick_));
	}
	delayed_send(PID_CLOCK, msg, 1000);
}

static void clock_handle_message(struct msgbuf *cmd)
{
	char c;
	unsigned int h, m, s;
	cmd->mtext[MTEXT_MAXLEN] = '\0';
	int nread = sscanf(cmd->mtext, "%c %u:%u:%u", &c, &h, &m, &s);
	if (nread != (c == 'S' ? 4 : 1)) {
		c = '\0';
	}
	switch (c) {
		case 'R':
			/* The %WR command will reset the current wall clock time to 00:00:00, starts the clock
			 * running and causes display of the current wall clock time on the console CRT. The
			 * display will be updated every second.
			 */
			h = m = s = 0;
		case 'S':
			/* The %WS hh:mm:ss command sets the current wall clock time to hh:mm:ss, starts
			 * the clock running and causes display of the current wall clock time on the console
			 * CRT. The display will be updated every second.
			 */
			clock_h = h;
			clock_m = m;
			clock_s = s;
			clock_handle_tick(NULL);
			break;
		case 'T':
			/* The %WT command will cause the wall clock display to be terminated.
			 */
			++clock_tick;
			break;
		default:
			printf("Invalid command: %s\n", cmd->mtext);
			break;
	}
}

/**
 * Wall clock display process
 */
void proc_clock(void)
{
	kcd_register("%W");

	for (;;) {
		int sender_id = -1;
		struct msgbuf *msg = receive_message(&sender_id);
		switch (sender_id) {
			case PID_KCD:
				clock_handle_message(msg);
				break;
			case PID_CLOCK:
				clock_handle_tick(msg);
				break;
		}
	}
}
