#include <assert.h>
#include <stddef.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#ifdef USR_CLOCK_TEST
//#include <stdlib.h>
#include <ucontext.h>
#include <unistd.h>
#include <queue>
#define _vsnprintf vsnprintf
#define _sscanf sscanf
typedef struct mem_t{
    // Guarantee 8-byte alignment, the largest common alignment
    int m_val[128 / 4] __attribute__ ((aligned (8)));
} mem_t;
#else
#include "rtx.h"
#include "sys_proc.h"
#include "printf.h"
#endif
#include "common.h"

// Wall clock display
// Test: g++ -o sys_proc sys_proc.c -DUSR_CLOCK_TEST -Wall -g3 && ./sys_proc

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

	struct msgbuf *p_msg_env = (struct msgbuf *)request_memory_block();
	p_msg_env->mtype = KCD_REG;
	strcpy(p_msg_env->mtext, cmd_prefix);
	send_message(PID_KCD, p_msg_env);
	p_msg_env = NULL;
}
#endif

static int crt_printf(const char *fmt, ...) {
	struct msgbuf *msg = (struct msgbuf *)request_memory_block();
	int ret;
	{
		va_list va;
		va_start(va, fmt);
		ret = _vsnprintf(msg->mtext, MTEXT_MAXLEN, fmt, va);
		msg->mtext[MTEXT_MAXLEN] = '\0';
		va_end(va);
	}
	msg->mtype = CRT_DISPLAY;
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
				if ((clock_h = (clock_h + 1) % 24) == 0) {
					// next day, do nothing
				}
			}
		}
		struct msgbuf *const msg2 = (struct msgbuf *)request_memory_block();
		memcpy(msg2, msg, 128);
		msg = msg2;
	} else {
		msg = (struct msgbuf *)request_memory_block();
		msg->mtype = DEFAULT;
	}

	crt_printf("Wall clock: %02u:%02u:%02u\n", clock_h, clock_m, clock_s);

	{
		int clock_tick_ = clock_tick;
		memcpy(msg->mtext, &clock_tick_, sizeof(clock_tick_));
	}
	delayed_send(PID_CLOCK, msg, 1000);
}

bool check_invalid_chars(char* cmd) {
    int idx_to_check[] = {4, 5, 7, 8, 10, 11};
 
    for (int i = 0; i < 6; i++) {
        char to_check = cmd[idx_to_check[i]];
        char s[] ={to_check, '\0'};
 
        if (to_check != '0' && atoi(s) == 0) {
            return false;
        }
    }
    return true;
}
 
static void clock_handle_message(struct msgbuf *cmd)
{
	char c;
	unsigned int h, m, s;
	cmd->mtext[MTEXT_MAXLEN] = '\0';
	// printf("COMMAND: %s\n", cmd->mtext);
	// printf("COMMAND SIZE: %d\n", strlen(cmd->mtext));
	if (strlen(cmd->mtext) < 3 || cmd->mtext[0] != '%' || cmd->mtext[1] != 'W') {
      printf("ERROR: Invalid command\n");
			return;
  }
	c = cmd->mtext[2];
	int expected_size = (c == 'S') ? 12 : 3;
	if (strlen(cmd->mtext) != expected_size) {
		printf("ERROR: Invalid command length or identifier\n");
		return;
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
				clock_h = h;
				clock_m = m;
				clock_s = s;
				++clock_tick;
				clock_handle_tick(NULL);
				break;
      case 'S': {
			/* The %WS hh:mm:ss command sets the current wall clock time to hh:mm:ss, starts
			 * the clock running and causes display of the current wall clock time on the console
			 * CRT. The display will be updated every second.
			 */
          if (cmd->mtext[3] != ' ' || cmd->mtext[6] != ':' || cmd->mtext[9] != ':') {
              printf("ERROR: Wrong command format\n");
              break;
          }
 
          char hStr[] = {cmd->mtext[4],  cmd->mtext[5], '\0'};
          char mStr[] = {cmd->mtext[7],  cmd->mtext[8], '\0'};
          char sStr[] = {cmd->mtext[10], cmd->mtext[11], '\0'};
 
          h = atoi(hStr);
          m = atoi(mStr);
          s = atoi(sStr);
 
					if (!check_invalid_chars(cmd->mtext)) {
              printf("ERROR: Detected non-int characters\n");
              break; // detect non-int characters
          }
 
          if (h < 24 && m < 60 && s < 60) {
              clock_h = h;
              clock_m = m;
              clock_s = s;
						  ++clock_tick;
              clock_handle_tick(NULL);
              break;
          }
          printf("ERROR: Invalid wall clock display values %u:%u:%u\n", h, m, s);
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

#ifndef USR_CLOCK_TEST
/*
* Allowed PIDs: 
*/
void proc_set_prio(void)
{
	/*register this process with KCD as the handler for the %C command*/
	kcd_register("%C");
	
	/* start receiving and parsing messages */
	for(;;) {
		char pid_buf[3] = {'\0'}; //Holds the pid of the process which is max 2 chars and 1 for '\0'
		char priority_buf[2] = {'\0'};
		
		int pid = 0;
		int priority = 0;
		//int sender_id = -1;	//Uh?!?!? Why?
		int sender_id = NULL;
		int ret_val = 0;
		
		struct msgbuf *msg = receive_message(&sender_id);
		
		//the first two characters in the mtext array are %C
		if(msg->mtext[2] == ' '
			&& msg->mtext[3] >= '0' && msg->mtext[3] <= '9'
			&& msg->mtext[4] == ' '
      && msg->mtext[5] >= '0' && msg->mtext[5] <= '3'
      && msg->mtext[6] == '\0'){
				pid_buf[0] = msg->mtext[3];
        priority_buf[0] = msg->mtext[5];
			} 
		else if (msg->mtext[2] == ' '
      && msg->mtext[3] == '1'
      && msg->mtext[4] >= '0' && msg->mtext[4] <= '3'
      && msg->mtext[5] == ' '
      && msg->mtext[6] >= '0' && msg->mtext[6] <= '3'
      && msg->mtext[7] == '\0'){
				pid_buf[0] = msg->mtext[3];
        pid_buf[1] = msg->mtext[4];
        priority_buf[0] = msg->mtext[6];
			}
		else{
			// struct msgbuf *display_msg = (struct msgbuf *)request_memory_block();
			// display_msg->mtype = CRT_DISPLAY;
			printf("Error: illegal parameters: \n\r");
			// send_message(PID_CRT, display_msg);
			release_memory_block(msg);
			
			continue;
		}
		
		pid = atoi(pid_buf);
    priority = atoi(priority_buf);
        
    ret_val = set_process_priority(pid, priority);
        
    if (ret_val != RTX_OK) {
			struct msgbuf *display_msg = (struct msgbuf *)request_memory_block();
      display_msg->mtype = CRT_DISPLAY;
      printf("Error: illegal PID or priority.\n\r");
            
      send_message(PID_CRT, display_msg);
    }
    release_memory_block(msg);
	}
}
#endif

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
static struct msgbuf *test_msgbuf = NULL;
static std::queue<struct msgbuf *> test_delayed_msg;

static void test_input(const char *buf) {
	test_from = PID_KCD;
	test_msgbuf = (struct msgbuf *)request_memory_block();
	printf("Test input: %s\n", buf);
	strcpy(test_msgbuf->mtext, buf);
	swapcontext(&test_main, &test_clock);
}

static int send_message(int dst, struct msgbuf *msg) {
	assert(dst == PID_CRT);
	assert(msg->mtype == CRT_DISPLAY);
	printf("Test output: %s\n", msg->mtext);
	strncpy(test_last_line, msg->mtext, MTEXT_MAXLEN);
	return RTX_OK;
}

static int delayed_send(int dst, struct msgbuf *msg, int delay_ms) {
	assert(dst == PID_CLOCK);
	test_delayed_msg.push(msg);
	return RTX_OK;
}

static void test_sleep(void) {
	if (test_delayed_msg.empty()) {
		return;
	}

	printf("Sleeping...\n");
	test_from = PID_CLOCK;
	test_msgbuf = test_delayed_msg.front();
	test_delayed_msg.pop();
	swapcontext(&test_main, &test_clock);
}

static void test_sleep_ntimes(int n) {
	int saved_fd = -1;
	for (int i = 0; i < n; ++i) {
		if (i >= 3 && saved_fd == -1) {
			fflush(stdout);
			saved_fd = dup(1);
			freopen("/dev/null", "w", stdout);
		}
		test_sleep();
	}
	if (saved_fd != -1) {
		long skipped = ftell(stdout);
		fflush(stdout);
		printf("... (skipped %ld bytes)\n", skipped);
		dup2(saved_fd, 1);
		close(saved_fd);
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
	test_input("%WT");
	test_sleep();

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
	test_input("%WS 12:34");
	test_expect("");
	test_input("%WS 12:34:56 ");
	test_expect("");
	test_input("%WS 12:34:56@");
	test_expect("");
	test_input("%WS 12:34:56");
	test_expect("Wall clock: 12:34:56\n");

	printf("\x1b[1mTesting %%WR and %%WT format\x1b[0m\n");
	strcpy(test_last_line, "");
	test_input("%WR ");
	test_input("%WT ");
	test_input("%WR asdfasdf");
	test_expect("");
	// Terminated should not output anything
	test_input("%WT");
	test_expect("");

	printf("\x1b[32;1mAll passed!\x1b[0m\n");
}
#endif
