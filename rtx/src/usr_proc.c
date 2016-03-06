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
#include "rtx.h"
#include "k_process.h"
#include "usr_proc.h"
#include "printf.h"

#define NUM_TESTS 122
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
	}
}

void test_assert(int expected, const char *msg, int lineno) {
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
	test_assert(!!(expected), "See usr_proc.c", __LINE__) \

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
}

static void test_transition_impl(const char *from, const char *to, int lineno)
{
	disable_irq();
	// Silently allow for non-FIFO ordering
	const char *const initial_test_state = test_state;
	const char *prev_test_state = test_state;
	int tries = 0;
	for (int i = 0; i < NUM_PROCS * 2 && test_state != from; ++i) {
		enable_irq();
		++tries;
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
			"test_transition(%s, %s)\n  expected state %s, but\n  got %s (%s after %d tries)\n",
			from,
			to,
			from,
			initial_test_state,
			test_state,
			tries
		);
		assert(0);
	}
#endif
	test_assert(from == test_state, "from == test_state (OS scheduled wrong process)", lineno);
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

	printf("Releasing memory block 0x%08x\n", (unsigned long) cur);
	TEST_EXPECT(0, release_memory_block(cur));
}

static void *test_mem_request(void) {
	mem_block_t *cur = (mem_block_t *)request_memory_block();
	printf("Requested memory block 0x%08x\n", (unsigned long) cur);
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
	printf("Setting process %d priority to %d\n", pid, prio);
	return set_process_priority(pid, prio);
}

static int test_get_process_priority(int pid) {
	const int prio = get_process_priority(pid);
	printf("Process %d has priority %d\n", pid, prio);
	return prio;
}

#define PID_P1 1
#define PID_P2 2
#define PID_P3 3
#define PID_P4 4
#define PID_P5 5
#define PID_P6 6
#define MIN_MEM_BLOCKS 5

/**
 * @brief: a process that tests the RTX API
 */
void proc1(void)
{
	test_printf("START\n");
	test_printf("total %d tests\n", NUM_TESTS);

	// int test_release_processor();
	// This primitive transfers the control to the RTX (the calling process voluntarily releases
	// the processor). The invoking process remains ready to execute and is put at the end of the
	// ready queue of the same priority. Another process may possibly be selected for execution.
	test_transition(test_state, "Memory blocking");
	test_set_process_priority(PID_P1, LOW);
	// Due to preemption, we can't tell whether we're blocked.
	// Try to allocate a few blocks.
	while (test_state == "Memory blocking") {
		test_mem_request();
	}

	// We should have been unblocked
	test_transition("Memory unblocked", "Get priority");
	TEST_EXPECT(LOWEST, test_get_process_priority(PID_P1));
	TEST_EXPECT(LOWEST, test_get_process_priority(PID_P2));
	TEST_EXPECT(RTX_ERR, test_get_process_priority(-1));
	TEST_EXPECT(RTX_ERR, test_get_process_priority(MAX_PID + 1));
	TEST_EXPECT(4, test_get_process_priority(PID_NULL));

	test_transition("Get priority", "Set null priority");
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_NULL, -1));
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_NULL, 0));
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_NULL, 3));
	TEST_EXPECT(0, test_set_process_priority(PID_NULL, 4));

	test_transition("Set null priority", "Set user priority (no-op)");
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_P1, -1));
	TEST_EXPECT(RTX_ERR, test_set_process_priority(PID_P1, 4));
	TEST_EXPECT(0, test_set_process_priority(PID_P1, test_get_process_priority(PID_P1)));
	TEST_EXPECT(0, test_set_process_priority(PID_P2, test_get_process_priority(PID_P2)));

	test_transition("Set user priority (no-op)", "Set user priority (higher)");
	TEST_EXPECT(0, test_set_process_priority(PID_P2, MEDIUM));

	test_transition("Set user priority (inversion)", "Preempt (inversion)");
	TEST_EXPECT(0, test_set_process_priority(PID_P2, HIGH));
	test_mem_release();

	test_transition("Set user priority (lower)", "Release processor (max priority)");
	// We're priority LOW, so we should still run.
	TEST_EXPECT(0, test_release_processor());

	test_transition("Release processor (max priority)", "Resource contention (1 blocked)");
	test_mem_request();

	test_transition("Resource contention resolved", "Count blocks");
	int blocks = 0;
	while (test_mem_front != NULL) {
		test_mem_release();
		++blocks;
	}
	TEST_ASSERT(blocks >= 5);

	test_transition("Count blocks", "Send self message");
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

	test_transition("Send self message", "Send other message");
	test_set_process_priority(PID_P2, HIGH);
	test_set_process_priority(PID_P3, HIGH);
	test_transition("Send other message (2 and 3 blocked)", "Receive other message (2 and 3 blocked)");
	for (int i = 0; i < 2; ++i) {
		const static int pids[2] = {PID_P2, PID_P3};
		struct msgbuf *msg = (struct msgbuf *)request_memory_block();
		msg->mtype = 42;
		strcpy(msg->mtext, "42");
		TEST_EXPECT(0, send_message(pids[i], msg));
	}

	test_release_processor();
	test_transition("Send delayed message", "Receive delayed message");
	for (int i = 0; i < 3; ++i) {
		int from = -1;
		struct msgbuf *msg = receive_message(&from);
		TEST_EXPECT(PID_P2, from);
		TEST_EXPECT(10 + i, msg->mtype);
		char buf[MTEXT_MAXLEN + 1];
		snprintf(buf, MTEXT_MAXLEN, "%d", i);
		TEST_ASSERT(!strncmp(buf, msg->mtext, MTEXT_MAXLEN));
	}

	test_transition("Receive delayed message", "Test finished");

	TEST_EXPECT(0, changed_bytes);
	TEST_ASSERT(finished_proc >= 2);
	// Wait for proc{4,5,6} to "finish"
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

	test_transition("Memory blocking", "Memory blocked");
	test_set_process_priority(PID_P1, LOWEST);
	// Let's free enough memory
	test_mem_release();
	test_mem_release();
	test_transition("Memory blocked", "Memory unblocked");

	test_transition("Set user priority (higher)", "Set user priority (inversion)");
	for (int i = 0; i < 30 && test_state == "Set user priority (inversion)"; ++i) {
		test_mem_request();
	}

	test_transition("Preempt (inversion)", "Set priority preempt (failed)");
	TEST_EXPECT(0, test_set_process_priority(PID_P1, LOW));
	TEST_EXPECT(0, test_set_process_priority(PID_P2, MEDIUM));

	test_transition("Set priority preempt (failed)", "Set user priority (lower, tied)");
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOW));

	test_transition("Set user priority (lower, tied)", "Set user priority (lower)");
	// Move ourselves after proc3 in the ready queue
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOWEST));

	test_transition("Resource contention (1 and 3 blocked)", "Resource contention (3 and 1 blocked)");
	TEST_EXPECT(0, test_set_process_priority(PID_P3, HIGH));
	TEST_EXPECT(0, test_set_process_priority(PID_P1, LOWEST));
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOWEST));
	test_mem_release();

	test_transition("Send other message", "Send other message (2 blocked)");
	TEST_EXPECT(HIGH, test_get_process_priority(PID_P2));
	test_receive_42_from_proc1();
	test_transition("Receive other message (2 and 3 blocked)", "Receive other message (3 blocked)");
	TEST_EXPECT(0, test_set_process_priority(PID_P2, LOWEST));

	test_transition("Receive other message (done)", "Send delayed message");
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

	test_transition("Resource contention (1 blocked)", "Resource contention (1 and 3 blocked)");
	test_mem_request();

	test_transition("Resource contention (3 and 1 blocked)", "Resource contention (1 blocked again)");
	test_mem_release();

	test_transition("Resource contention (1 blocked again)", "Resource contention resolved");
	TEST_EXPECT(0, test_set_process_priority(PID_P3, LOWEST));

	test_transition("Send other message (2 blocked)", "Send other message (2 and 3 blocked)");
	TEST_EXPECT(HIGH, test_get_process_priority(PID_P2));
	test_receive_42_from_proc1();
	test_transition("Receive other message (3 blocked)", "Receive other message (done)");
	TEST_EXPECT(0, test_set_process_priority(PID_P3, LOWEST));

	++finished_proc;
	TEST_EXPECT(0, test_release_processor());

	infinite_loop();
}

void proc4(void)
{
	infinite_loop();
}

void proc5(void)
{
	infinite_loop();
}

void proc6(void)
{
	infinite_loop();
}
