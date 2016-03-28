/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/02/28
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include "rtx.h"
#include "common.h"
#include "k_process.h"
#include "k_cycle_count.h"
#include "usr_proc.h"
#include "printf.h"
#include "list.h"

#ifdef HAS_TIMESLICING
#define NUM_TESTS 122
#else
// Test FIFO ordering
#define NUM_TESTS 147
#endif
#define GROUP_ID "004"

#define test_printf(...) printf("G" GROUP_ID "_test: " __VA_ARGS__)

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];
static volatile int proc2_work_remaining = 3, proc3_work_remaining = 3, finished_proc = 0, finished = 0;
static int tests_ran = 0, tests_failed = 0;
const char *test_state = "Starting tests";

static void infinite_loop(void)
{
	for (;;) {
		release_processor();
	}
}

#ifdef LOW_MEM
void test_assert(int expected, int lineno) {
	const char *msg = "See usr_proc.c";
#else
void test_assert(int expected, const char *msg, int lineno) {
#endif
	if (finished) {
		infinite_loop();
	}
	const char *status = expected ? "OK" : "FAIL";
	if (!expected) {
		++tests_failed;
		#ifdef DEBUG_0
			printf("Assertion failed in %s:%d: %s\n", __FILE__, lineno, msg);
		#endif
	}
	test_printf("test %d %s\n", ++tests_ran, status);
}

#ifdef LOW_MEM
#define TEST_ASSERT(expected) \
	test_assert(!!(expected), __LINE__) \

#else
#define TEST_ASSERT(expected) \
	test_assert(!!(expected), #expected, __LINE__); \

#endif

#define TEST_EXPECT(expected, actual) TEST_ASSERT((expected) == (actual));

static int test_release_processor_impl(const char *procname) {
	printf("Unscheduling %s\n", procname);
	int ret = release_processor();
	if (finished) {
		infinite_loop();
	}
	printf("Rescheduling %s\n", procname);
	return ret;
}
#define test_release_processor() test_release_processor_impl(__FUNCTION__)
void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x200;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
	g_test_procs[3].mpf_start_pc = &proc4;
	g_test_procs[4].mpf_start_pc = &proc5;
	g_test_procs[5].mpf_start_pc = &proc6;
	g_test_procs[6].mpf_start_pc = &proc_A;
	g_test_procs[7].mpf_start_pc = &proc_B;
	g_test_procs[8].mpf_start_pc = &proc_C;
}

static void test_transition_impl(const char *from, const char *to, int lineno)
{
	disable_irq();
	// Silently allow for non-FIFO ordering, up to MAX_YIELDS times
#ifdef HAS_TIMESLICING
#define MAX_YIELDS (NUM_PROCS * 2)
#else
#define MAX_YIELDS (0)
#endif
	const char *const initial_test_state = test_state;
	const char *prev_test_state = test_state;
	int yields = 0;
	for (int i = 0; i < MAX_YIELDS && test_state != from; ++i) {
		enable_irq();
		++yields;
		release_processor();
		disable_irq();
		if (prev_test_state != test_state) {
			prev_test_state = test_state;
			i = 0;
		}
	}
#ifdef DEBUG_0
	if (from != test_state) {
		printf(
			"test_transition(%s, %s)\n  expected state %s, but\n  got %s (%s after %d yields)\n",
			from,
			to,
			from,
			initial_test_state,
			test_state,
			yields
		);
		assert(0);
	}
#endif
#ifdef LOW_MEM
	test_assert(from == test_state, lineno);
#else
	test_assert(from == test_state, "from == test_state (OS scheduled wrong process)", lineno);
#endif
	printf("Done: %s, starting: %s at %s:%d\n", test_state, to, __FILE__, lineno);
	test_state = to;
	enable_irq();
}
#define test_transition(from, to) test_transition_impl((from), (to), __LINE__)

typedef struct mem_block {
	struct mem_block *volatile next;
	volatile char data[128 - sizeof(struct mem_block *)];
} mem_block_t;
mem_block_t *volatile test_mem_front = NULL;
char test_mem_blocks = 0;
// Count this as a single test
int changed_bytes = 0;

static void test_mem_release(void) {
	assert(test_mem_front != NULL);
	mem_block_t *cur = test_mem_front;
	test_mem_front = cur->next;

	for (int i = 0; i < sizeof(cur->data); ++i) {
		if (cur->data[i] != i + test_mem_blocks) {
			++changed_bytes;
		}
	}
	--test_mem_blocks;

	printf("Releasing mem block 0x%08x\n", (unsigned long) cur);
	TEST_EXPECT(0, release_memory_block(cur));
}

static void *test_mem_request(void) {
	mem_block_t *cur = (mem_block_t *)request_memory_block();
	printf("Requested mem block 0x%08x\n", (unsigned long) cur);
	assert(sizeof(*cur) == 128);

	++test_mem_blocks;
	for (int i = 0; i < sizeof(cur->data); ++i) {
		cur->data[i] = i + test_mem_blocks;
	}

	cur->next = test_mem_front;
	test_mem_front = cur;
	return (void *) cur->data;
}

static int test_set_process_priority(int pid, int prio) {
	printf("Setting process %d prio to %d\n", pid, prio);
	return set_process_priority(pid, prio);
}

static int test_get_process_priority(int pid) {
	const int prio = get_process_priority(pid);
	printf("Process %d has prio %d\n", pid, prio);
	return prio;
}

#define TEST_TIME(stmt) do {\
	unsigned before = get_cycle_count24();\
	stmt;\
	unsigned after = get_cycle_count24();\
	printf(\
		"%s: \"%s\" took %u cycles at %u Hz\n",\
		__FUNCTION__,\
		#stmt,\
		cycle_count_difference(before, after),\
		__CORE_CLK\
	);\
} while (0)

static void test_time_primitives(int caller_pid) {
	const int iterations = 3;
	printf("Testing primitives, running %d iterations\n", iterations);
	for (int iteration = 0; iteration < iterations; ++iteration) {
		// Test doing nothing
		TEST_TIME();

		// Test request_memory_block
		MSG_BUF *p;
		TEST_TIME(p = request_memory_block());

		// Test send_message
		p->mtype = DEFAULT;
		TEST_TIME(send_message(caller_pid, p));
		
		// Test receive_message
		int sender = -1;
		MSG_BUF *q;
		TEST_TIME(q = receive_message(&sender));
		assert(p == q);
		
		release_memory_block(p);
		p = q = NULL;
	}
}

#define MIN_MEM_BLOCKS 5

/**
 * @brief: a process that tests the RTX API
 */
void proc1(void)
{
	test_printf("START\n");
	test_printf("total %d tests\n", NUM_TESTS);

#ifdef HAS_TIMESLICING
	test_transition(test_state, "Memory blocking");
	test_set_process_priority(PID_P1, LOW);
	// Due to preemption, we can't tell whether we're blocked.
	// Try to allocate a few blocks.
	while (test_state == "Memory blocking") {
		test_mem_request();
	}

	// We should have been unblocked
	test_transition("Memory unblocked", "Get prio");
#else
	// int test_release_processor();
	// This primitive transfers the control to the RTX (the calling process voluntarily releases
	// the processor). The invoking process remains ready to execute and is put at the end of the
	// ready queue of the same priority. Another process may possibly be selected for execution.
	test_transition(test_state, "FIFO scheduling");
	assert(proc2_work_remaining == 3);
	for (int i = 2; i >= 0; --i) {
		TEST_EXPECT(0, test_release_processor());
		TEST_EXPECT(i, proc2_work_remaining);
		TEST_EXPECT(i, proc3_work_remaining);
	}

	test_transition("FIFO scheduling", "Equal prio mem blocking");
	while (test_state == "Equal prio mem blocking") {
		test_mem_request();
	}

	// We should have been unblocked
	test_transition("Equal prio mem unblocked", "Get prio");
#endif
	TEST_EXPECT(LOWEST, test_get_process_priority(PID_P1));
	TEST_EXPECT(LOWEST, test_get_process_priority(PID_P2));
	TEST_EXPECT(RTX_ERR, test_get_process_priority(-1));
	TEST_EXPECT(RTX_ERR, test_get_process_priority(MAX_PID + 1));
	TEST_EXPECT(4, test_get_process_priority(PID_NULL));

	test_transition("Get prio", "Set null prio");
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_NULL, -1));
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_NULL, 0));
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_NULL, 3));
	TEST_EXPECT(0, test_set_process_priority(PID_NULL, 4));

	test_transition("Set null prio", "Set user prio (no-op)");
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_P1, -1));
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_P1, 4));
	TEST_EXPECT(0, test_set_process_priority(PID_P1, test_get_process_priority(PID_P1)));
	TEST_EXPECT(0, test_set_process_priority(PID_P2, test_get_process_priority(PID_P2)));

	test_transition("Set user prio (no-op)", "Set user prio (higher)");
	TEST_EXPECT(0, test_set_process_priority(PID_P2, MEDIUM));

#ifdef HAS_TIMESLICING
	test_transition("Set user prio (inversion)", "Preempt (inversion)");
#else
	test_transition("Set user prio (inversion 2)", "Preempt (inversion)");
#endif
	TEST_EXPECT(0, test_set_process_priority(PID_P2, HIGH));
	test_mem_release();

	test_transition("Set user prio (lower)", "Release processor (max prio)");
	// We're prio LOW, so we should still run.
	TEST_EXPECT(0, test_release_processor());

	test_transition("Release processor (max prio)", "Resource contention (1 blocked)");
	test_mem_request();

	test_transition("Resource contention resolved", "Count blocks");
	int blocks = 0;
	while (test_mem_front != NULL) {
		test_mem_release();
		++blocks;
	}
	TEST_ASSERT(blocks >= 5);

	test_transition("Count blocks", "Send self msg");
	{
		char msg_buf[128];
		struct msgbuf *msg = (struct msgbuf *)msg_buf;
		msg->mtype = DEFAULT;
		strcpy(msg->mtext, "Hi");
		TEST_EXPECT(0, !send_message(-1, msg));
		TEST_EXPECT(0, delayed_send(PID_P1, msg, 0));

		{
			int from = -1;
			struct msgbuf *m1 = receive_message(&from);
			TEST_EXPECT(PID_P1, from);
			TEST_EXPECT(msg, m1);
			TEST_ASSERT(!strcmp("Hi", m1->mtext));
		}

		{
			TEST_EXPECT(0, send_message(PID_P1, msg));
			struct msgbuf *m2 = receive_message(NULL);
			TEST_ASSERT(!strcmp("Hi", m2->mtext));
		}
	}

	test_transition("Send self msg", "Send other msg");
	test_set_process_priority(PID_P2, HIGH);
	test_set_process_priority(PID_P3, HIGH);
	test_transition("Send other msg (2 and 3 blocked)", "Recv other msg (2 and 3 blocked)");
	for (int i = 0; i < 2; ++i) {
		const static int pids[2] = {PID_P2, PID_P3};
		struct msgbuf *msg = (struct msgbuf *)request_memory_block();
		msg->mtype = 42;
		strcpy(msg->mtext, "42");
		TEST_EXPECT(0, send_message(pids[i], msg));
	}

	test_release_processor();
	test_transition("Send delayed msg", "Recv delayed msg");
	for (int i = 0; i < 3; ++i) {
		int from = -1;
		struct msgbuf *msg = receive_message(&from);
		TEST_EXPECT(PID_P2, from);
		TEST_EXPECT(10 + i, msg->mtype);
		char buf[MTEXT_MAXLEN + 1];
		snprintf(buf, MTEXT_MAXLEN, "%d", i);
		TEST_ASSERT(!strncmp(buf, msg->mtext, MTEXT_MAXLEN));
	}

	test_transition("Recv delayed msg", "Tests done");

	TEST_EXPECT(0, changed_bytes);
	TEST_ASSERT(finished_proc >= 2);
	while (finished_proc < 2) {
		test_release_processor();
	}

	test_printf("%d/%d tests OK\n", tests_ran - tests_failed, tests_ran);
	test_printf("%d/%d tests FAIL\n", tests_failed, tests_ran);
	// This invalidates tests_ran, but since all processes have reached
	//   infinite_loop(), everything is okay.
	assert(NUM_TESTS == tests_ran);
	test_printf("END\n");
	finished = 1;
	
	test_time_primitives(PID_P1);
	infinite_loop();
}

static void test_receive_42_from_proc1(void)
{
	int from = -1;
	struct msgbuf *msg = receive_message(&from);
	TEST_EXPECT(PID_P1, from);
	TEST_EXPECT(42, msg->mtype);
	TEST_ASSERT(!strcmp("42", msg->mtext));
	release_memory_block(msg);
}

/**
 * @brief: a helper process to proc1
 */
void proc2(void)
{
	printf("Started %s\n", __FUNCTION__);
#ifdef HAS_TIMESLICING
	test_transition("Memory blocking", "Memory blocked");
	test_set_process_priority(PID_P1, LOWEST);

	// Let's free enough memory
	test_mem_release();
	test_mem_release();
	test_transition("Memory blocked", "Memory unblocked");

	test_transition("Set user prio (higher)", "Set user prio (inversion)");
	for (int i = 0; i < 30 && test_state == "Set user prio (inversion)"; ++i) {
		test_mem_request();
	}
#else
	while (proc2_work_remaining > 0) {
		TEST_EXPECT(proc3_work_remaining, proc2_work_remaining);
		--proc2_work_remaining;
		TEST_EXPECT(0, test_release_processor());
	}
	test_transition("Equal prio mem blocking", "Equal prio mem unblocking");

	// Let's free enough memory
	test_mem_release();
	test_mem_release();

	test_transition("Equal prio mem unblocking", "Equal prio mem unblocking 2");
	// Should schedule proc3 since proc1 was preempted recently
	TEST_EXPECT(0, test_release_processor());

	test_transition("Set user prio (higher)", "Set user prio (inversion)");
	for (int i = 0; i < 3 && test_state == "Set user prio (inversion)"; ++i) {
		test_mem_request();
	}
#endif

	test_transition("Preempt (inversion)", "Set prio preempt (failed)");
	TEST_EXPECT(0, test_set_process_priority(PID_P1, LOW));
	TEST_EXPECT(0, test_set_process_priority(PID_P2, MEDIUM));

	test_transition("Set prio preempt (failed)", "Set user prio (lower, tied)");
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOW));

	test_transition("Set user prio (lower, tied)", "Set user prio (lower)");
	// Move ourselves after proc3 in the ready queue
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOWEST));

	test_transition("Resource contention (1 and 3 blocked)", "Resource contention (3 and 1 blocked)");
	TEST_EXPECT(0, test_set_process_priority(PID_P3, HIGH));
	TEST_EXPECT(0, test_set_process_priority(PID_P1, LOWEST));
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOWEST));
	test_mem_release();

	test_transition("Send other msg", "Send other msg (2 blocked)");
	TEST_EXPECT(HIGH, test_get_process_priority(PID_P2));
	test_receive_42_from_proc1();
	test_transition("Recv other msg (2 and 3 blocked)", "Recv other msg (3 blocked)");
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOWEST));
	release_processor(); // Go to proc2

	test_transition("Recv other msg (done)", "Send delayed msg");
	for (int i = 2; i >= 0; --i) {
		struct msgbuf *msg = (struct msgbuf *)request_memory_block();
		msg->mtype = 10 + i;
		sprintf(msg->mtext, "%d", i);
		TEST_EXPECT(0, delayed_send(PID_P1, msg, i * 100));
	}

	++finished_proc;

	infinite_loop();
}

/**
 * A second helper process.
 */
void proc3(void)
{
	printf("Started %s\n", __FUNCTION__);
#ifndef HAS_TIMESLICING
	while (proc3_work_remaining > 0) {
		--proc3_work_remaining;
		TEST_EXPECT(proc2_work_remaining, proc3_work_remaining);
		TEST_EXPECT(0, test_release_processor());
	}

	// Since proc1 was preempted, it's at the back of the ready queue
	// Let's run it.
	test_transition("Equal prio mem unblocking 2", "Equal prio mem unblocked");
	test_release_processor();

	test_transition("Set user prio (inversion)", "Set user prio (inversion 2)");
	test_release_processor();
#endif

	test_transition("Resource contention (1 blocked)", "Resource contention (1 and 3 blocked)");
	test_mem_request();

	test_transition("Resource contention (3 and 1 blocked)", "Resource contention (1 blocked again)");
	test_mem_release();

	test_transition("Resource contention (1 blocked again)", "Resource contention resolved");
	TEST_EXPECT(0, test_set_process_priority(PID_P3, LOWEST));
	release_processor(); // Go back to proc1

	test_transition("Send other msg (2 blocked)", "Send other msg (2 and 3 blocked)");
	TEST_EXPECT(HIGH, test_get_process_priority(PID_P2));
	test_receive_42_from_proc1();
	test_transition("Recv other msg (3 blocked)", "Recv other msg (done)");
	TEST_EXPECT(0, test_set_process_priority(PID_P3, LOWEST));

	++finished_proc;
	TEST_EXPECT(0, test_release_processor());

	infinite_loop();
}

// Process to show blocked on receive state hotkey
void proc4(void)
{
	for (;;) {
		receive_message(NULL);
	}
}

// Process to show ready state hotkey
void proc5(void)
{
	infinite_loop();
}

// Process to show blocked on memory hotkey
void proc6(void)
{
	for (;;) {
		infinite_loop();
	}
}

/********************* Stress Test Procs ***********************/
void proc_A(void)
{
	// Process A:
	// p <- request_memory_block
  // register with Command Decoder as handler of %Z commands
	struct msgbuf *p_msg_env = (struct msgbuf *)request_memory_block();
	p_msg_env->mtype = KCD_REG;
	strcpy(p_msg_env->mtext, "%Z");
	send_message(PID_KCD, p_msg_env);

	for (;;) {
		// p <- receive a message
		// if the message(p) contains the %Z command then
		//   release_memory_block(p)
		//   exit the loop
		// else
		//   release_memory_block(p)
		// endif
		p_msg_env = receive_message(NULL);
		if(strstr(p_msg_env->mtext, "%Z") != NULL) {
			release_memory_block(p_msg_env);
			break;
		} else {
			release_memory_block(p_msg_env);
		}
	}
	printf("Got command %Z\n");
	// Second loop:
	// num = 0
	int num = 0;
	for (;;) {
		// p <- request memory block to be used as a message envelope
		// set message_type field of p to count_report
		// set msg_data[0] field of p to num
		// send the message(p) to process B
		// num = num + 1
		// release_processor()
		p_msg_env = (struct msgbuf *)request_memory_block();
		p_msg_env->mtype = COUNT_REPORT;
		_sprintf(p_msg_env->mtext, "%d", num);
		printf("proc_A sending message #%d\n", num);
		send_message(PID_B, p_msg_env);
		printf("proc_A sent message #%d\n", num);
		num++;
		release_processor();
	}
	// note that Process A does not de-allocate
	// any received envelopes in the second loop
}

void proc_B(void)
{
	// loop forever
	// receive a message
	// send the message to process C
	// endloop
	for (;;) {
		struct msgbuf *msg = receive_message(NULL);
		printf("proc_B sending message to proc_C: %s\n", msg->mtext);
		send_message(PID_C, msg);
	}
}

LL_DECLARE(static c_message_queue, MSG_BUF *, NUM_MEM_BLOCKS + 2);

/**
 * Hibernate for 10 seconds.
 * Takes a message envelope (memory block) q and sends it to itself.
 * `hibernate` returns after the message has been received again.
 * Does not free `q`.
 * Note: hibernate changes the message mtype
 */
static void hibernate(MSG_BUF *q) {
	// request a delayed_send for 10 sec delay with msg_type=wakeup10 using q
	q->mtype = WAKEUP_10;
	delayed_send(PID_C, q, 10000);

	// loop forever
	// p <- receive a message //block and let other processes execute
	//   if (message_type of p == wakeup10) then
	//     exit this loop
	//   else
	//     put message (p) on the local message queue for later processing
	//   endif
	// endloop
	for (;;) {
		struct msgbuf* p = (struct msgbuf*)receive_message(NULL);
		if(p->mtype == WAKEUP_10) {
			assert(p == q);
			break;
		} else {
			LL_PUSH_BACK(c_message_queue, p);
		}
	}
}

void proc_C(void) {
	// perform any needed initialization and create a local message queue
	// If process A has higher priority, then we could be out of
	//   memory blocks right now. So, we can't request memory here.
	bool hibernate_for_10_sec = false;

	for(;;) {
		struct msgbuf* msg = NULL;
		// if (local message queue is empty) then
		//  	p <- receive a message
		// else
		//	 p <- dequeue the first message from the local message queue
		// endif
		if (LL_SIZE(c_message_queue) == 0) {
			msg = receive_message(NULL);	
		} else {
			msg = LL_POP_FRONT(c_message_queue);
		}

		// if msg_type of p == count_report then
		//   if msg_data[0] of p is evenly divisible by 20 then
		//     send "Process C" to CRT display using msg envelope p
		//     hibernate for 10 sec
		//   endif
		// endif
		if (msg->mtype == COUNT_REPORT) {
			int count = atoi(msg->mtext);

			if (hibernate_for_10_sec) {
				hibernate_for_10_sec = false;
				hibernate(msg);
			}

			if (count % 20 == 0) {
				msg->mtype = CRT_DISPLAY;
				strcpy(msg->mtext, "Process C\n");
				send_message(PID_CRT, msg);
				msg = NULL;

				hibernate_for_10_sec = true;
				// Don't free the memory block, since it's set
				continue;
			}
		}
		
		release_memory_block(msg);
		release_processor();
	}
}
