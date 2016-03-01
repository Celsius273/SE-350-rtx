#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#ifdef USR_CLOCK_TEST
#include <stdlib.h>
#include <ucontext.h>
typedef struct mem_t{
    // Guarantee 8-byte alignment, the largest common alignment
    int m_val[128 / 4] __attribute__ ((aligned (8)));
} mem_t;
#else
#include "k_memory.h"
#include "usr_clock.h"
#endif
#include "common.h"

// Wall clock display
// Test: gcc -o usr_clock usr_clock.c -DUSR_CLOCK_TEST -Wall -g3 && ./usr_clock

#define MTEXT_MAXLEN (sizeof(mem_t) - offsetof(struct msgbuf, mtext) - 1)

#ifdef USR_CLOCK_TEST

static const char *clock_test_prefix = NULL;
static void kcd_register(const char *cmd_prefix) {
	assert(clock_test_prefix == NULL);
	clock_test_prefix = cmd_prefix;
}

static int send_message(int dst, struct msgbuf *msg);

static int delayed_send(int dst, struct msgbuf *msg, int delay_ms);

struct msgbuf *receive_message(int *from);

#define request_memory_block() malloc(128)

#define release_memory_block(x) free((x))

#else

static void kcd_register(const char *cmd_prefix)
{
	assert(strlen(cmd_prefix) <= MTEXT_MAXLEN);

	struct msgbuf *p_msg_env = request_memory_block();
	p_msg_env->mtype = KCD_REG;
	strcpy(p_msg_env->mtext, cmd_prefix);
	send_message(PID_KCD, p_msg_env);
	p_msg_env = NULL;
}
#endif

static int crt_printf(const char *fmt, ...) {
	struct msgbuf *msg = request_memory_block();
	int ret;
	{
		va_list va;
		va_start(va, fmt);
		ret = vsnprintf(msg->mtext, MTEXT_MAXLEN, fmt, va);
		msg->mtext[MTEXT_MAXLEN] = '\0';
		va_end(va);
	}
	send_message(PID_CRT, msg);
	return ret;
}

volatile int clock_tick = 0;
volatile unsigned int clock_h, clock_m, clock_s;

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
		msg = memcpy(request_memory_block(), msg, 128);
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
	int nread = sscanf(cmd->mtext, "%%W%c %u:%u:%u", &c, &h, &m, &s);
	if (nread < 1 || nread != (c == 'S' ? 4 : 1)) {
		c = '\0';
	}
	switch (c) {
		case 'T':
			/* The %WT command will cause the wall clock display to be terminated.
			 */
			++clock_tick;
			break;
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
			if (
					0 <= h && h < 24 &&
					0 <= m && m < 60 &&
					0 <= s && s < 60
			   ) {
				clock_h = h;
				clock_m = m;
				clock_s = s;
				clock_handle_tick(NULL);
				break;
			}
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
		release_memory_block(msg);
	}
}

#ifdef USR_CLOCK_TEST
static char test_last_line[MTEXT_MAXLEN + 1];

static void test_expect(const char *buf) {
	if (strncmp(test_last_line, buf, sizeof(test_last_line))) {
		printf("Expected: %s, got: %s\n", buf, test_last_line);
		assert(false);
	}
}

static ucontext_t test_clock, test_main;
static int test_from;
static struct msgbuf *test_msgbuf = NULL, *test_delayed_msg = NULL;

static void test_input(const char *buf) {
	test_from = PID_KCD;
	test_msgbuf = request_memory_block();
	printf("Test input: %s\n", buf);
	strcpy(test_msgbuf->mtext, buf);
	swapcontext(&test_main, &test_clock);
}

static int send_message(int dst, struct msgbuf *msg) {
	assert(dst == PID_CRT);
	printf("Test output: %s\n", msg->mtext);
	strncpy(test_last_line, msg->mtext, MTEXT_MAXLEN);
	return RTX_OK;
}

static int delayed_send(int dst, struct msgbuf *msg, int delay_ms) {
	assert(dst == PID_CLOCK);
	assert(test_delayed_msg == NULL);
	test_delayed_msg = msg;
	return RTX_OK;
}

static void test_sleep(void) {
	if (!test_delayed_msg) {
		return;
	}

	printf("Sleeping...\n");
	test_from = PID_CLOCK;
	test_msgbuf = test_delayed_msg;
	test_delayed_msg = NULL;
	swapcontext(&test_main, &test_clock);
}

static void test_sleep_ntimes(int n) {
	for (int i = 0; i < n; ++i) {
		test_sleep();
	}
}

struct msgbuf *receive_message(int *from) {
	while (!test_msgbuf) {
		swapcontext(&test_clock, &test_main);
	}
	struct msgbuf *ret = test_msgbuf;
	test_msgbuf = NULL;
	*from = test_from;

	char x = ret->mtext[0]; // trigger valgrind
	x = x;
	return ret;
}

int main(void)
{
	getcontext(&test_clock);
	test_clock.uc_link = 0;
	test_clock.uc_stack.ss_size = SIGSTKSZ;
	test_clock.uc_stack.ss_sp = malloc(test_clock.uc_stack.ss_size);
	test_clock.uc_stack.ss_flags = 0;
	makecontext(&test_clock, proc_clock, 0);

	printf("\x1b[1mTesting invalid command\x1b[0m\n");
	test_input("%W");
	test_expect("");

	printf("\x1b[1mTesting %%WR\x1b[0m\n");
	test_input("%WR");
	test_expect("Wall clock: 00:00:00\n");
	test_sleep();
	test_expect("Wall clock: 00:00:01\n");
	test_sleep();
	test_sleep();
	test_expect("Wall clock: 00:00:03\n");

	printf("\x1b[1mTesting %%WS\x1b[0m\n");
	test_input("%WS 12:34:56");
	test_expect("Wall clock: 12:34:56\n");
	test_sleep_ntimes(4);
	test_expect("Wall clock: 12:35:00\n");
	test_sleep_ntimes(25 * 60);
	test_expect("Wall clock: 13:00:00\n");
	test_sleep_ntimes(11 * 60 * 60);
	test_expect("Wall clock: 00:00:00\n");

	printf("\x1b[1mTesting time format\x1b[0m\n");
	strcpy(test_last_line, "");
	test_input("%WS 99:99:99");
	test_expect("");
	test_input("%WS hello world");
	test_expect("");
	test_input("%WS 12:34:567");
	test_expect("");
	test_input("%WS 12:34:56@"); // "valid"
	test_expect("Wall clock: 12:34:56\n");
}
#endif
