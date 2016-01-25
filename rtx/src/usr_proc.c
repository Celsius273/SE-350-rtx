/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/02/28
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "k_process.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "printf.h"

// TODO set group id
#define test_printf(...) printf("Gid_test: " __VA_ARGS__)

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];
static int proc2_work_remaining = 3, proc3_work_remaining = 3, finished_procs = 0;
static int tests_ran = 0, tests_failed = 0;
const char *test_state = "Starting tests";

void test_assert(int expected, const char *msg, int lineno) {
	const char *status = expected ? "OK" : "FAIL";
	if (!expected) {
		++tests_failed;
	}
	test_printf("test %d %s\n", ++tests_ran, status);
#ifdef DEBUG_0
	printf("Assertion failed in %s:%d: %s\n", __FILE__, lineno, msg);
#endif
}

#define TEST_EXPECT(expected) \
	test_assert(!!(expected), #expected, __LINE__); \

#define TEST_EXPECT(expected, actual) TEST_EXPECT((expected) == (actual));

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
}

static void test_transition(const char *from, const char *to)
{
	TEST_EXPECT(from, test_state);
	test_state = to;
}

void infinite_loop(void)
{
	for (;;) {
		release_processor();
	}
}

typedef struct mem_block {
	struct mem_block *volatile next;
	volatile char data[128 - sizeof(next)];
} mem_block_t;
mem_block_t *volatile test_mem_front = NULL;
char test_mem_blocks = 0;
// Count this as a single test
int changed_bytes = 0;

void test_mem_release(void) {
	assert(test_mem_front != NULL);
	mem_block_t *cur = test_mem_front;
	test_mem_front = cur->next;

	for (int i = 0; i < sizeof(cur->data); ++i) {
		if (cur->data[i] != i + test_mem_blocks) {
			++changed_bytes;
		}
	}
	--test_mem_blocks;

	TEST_EXPECT(0, release_memory_block(test_mem_front));
}

void *test_mem_request(void) {
	mem_block_t *cur = (mem_block_t *)request_memory_block();
	assert(sizeof(*cur) == 128);

	++test_mem_blocks;
	for (int i = 0; i < sizeof(cur->data); ++i) {
		cur->data[i] = i + test_mem_blocks;
	}

	cur->next = test_mem_front;
	test_mem_front = cur;
}

#define PROC1_PID 1
#define PROC2_PID 2
#define PROC3_PID 3

/**
 * @brief: a process that tests the RTX API
 */
void proc1(void)
{
	test_printf("START\n");

	// int release_processor();
	// This primitive transfers the control to the RTX (the calling process voluntarily releases
	// the processor). The invoking process remains ready to execute and is put at the end of the
	// ready queue of the same priority. Another process may possibly be selected for execution.
	test_transition("initial", "FIFO scheduling")
	assert(proc2_work_remaining == 3);
	for (int i = 2; i >= 0; --i) {
		TEST_EXPECT(0, release_processor());
		TEST_EXPECT(i, proc2_work_remaining);
		TEST_EXPECT(i, proc3_work_remaining);
	}

	test_transition("FIFO scheduling", "Equal priority memory blocking");
	while (test_state == "Equal priority memory blocking") {
		test_mem_request();
	}

	test_transition("Equal priority memory unblocked", "Get priority");
	TEST_EXPECT(LOWEST, get_process_priority(PROC1_PID));
	TEST_EXPECT(LOWEST, get_process_priority(PROC2_PID));
	TEST_EXPECT(RTX_ERR, get_process_priority(-1));
	TEST_EXPECT(RTX_ERR, get_process_priority(NUM_TEST_PROCS + 1));
	TEST_EXPECT(4, get_process_priority(NULL_PID));

	test_transition("Get priority", "Set null priority");
	TEST_EXPECT(RTX_ERR, set_process_priority(NULL_PID, -1));
	TEST_EXPECT(RTX_ERR, set_process_priority(NULL_PID, 0));
	TEST_EXPECT(RTX_ERR, set_process_priority(NULL_PID, 3));
	TEST_EXPECT(0, set_process_priority(NULL_PID, 4));

	test_transition("Set null priority", "Set user priority (no-op)");
	TEST_EXPECT(RTX_ERR, set_process_priority(PROC1_PID, -1));
	TEST_EXPECT(RTX_ERR, set_process_priority(PROC1_PID, 4));
	TEST_EXPECT(0, set_process_priority(PROC1_PID, get_process_priority(PROC1_PID)));
	TEST_EXPECT(0, set_process_priority(PROC2_PID, get_process_priority(PROC2_PID)));

	test_transition("Set user priority (no-op)", "Set user priority (higher)");
	TEST_EXPECT(0, set_process_priority(PROC2_PID, MEDIUM));

	test_transition("Set user priority (inversion 2)", "Preempt (inversion)");
	TEST_EXPECT(0, set_process_priority(PROC2_PID, HIGH));
	test_mem_release();

	test_transition("Set user priority (lower)", "Release processor (max priority)");
	TEST_EXPECT(0, release_processor());

	test_transition("Release processor (max priority)", "Resource contention (1 blocked)");
	test_mem_request();

	test_transition("Resource contention resolved", "Count blocks");
	int blocks = 0;
	while (test_mem_front != NULL) {
		test_mem_release();
		++blocks;
	}
	TEST_EXPECT(blocks >= 30);
	test_transition("Count blocks", "Test finished");

	TEST_EXPECT(0, changed_bytes);
	TEST_EXPECT(2, finished_procs);

	test_printf("%d/%d tests OK\n", tests_ran - tests_failed, tests_ran);
	test_printf("%d/%d tests FAIL\n", tests_failed, tests_ran);
	test_printf("END\n");
	infinite_loop();
}

/**
 * @brief: a helper process to proc1
 */
void proc2(void)
{
	while (proc2_work_remaining > 0) {
		TEST_EXPECT(proc3_work_remaining, proc2_work_remaining);
		--proc2_work_remaining;
		TEST_EXPECT(0, release_processor());
	}

	test_transition("Equal priority memory blocking", "Equal priority memory unblocking");
	test_mem_release();

	test_transition("Equal priority memory unblocking", "Equal priority memory unblocked");
	TEST_EXPECT(0, release_processor());

	test_transition("Set user priority (higher)", "Set user priority (inversion)");
	test_mem_request();

	test_transition("Preempt (inversion)", "Set priority preempt (failed)");
	TEST_EXPECT(0, set_process_priority(PROC1_PID, LOW));
	TEST_EXPECT(0, set_process_priority(PROC2_PID, MEDIUM));

	test_transition("Set priority preempt (failed)", "Set user priority (lower, tied)");
	TEST_EXPECT(0, set_process_priority(PROC2_PID, LOW));

	test_transition("Set user priority (lower, tied)", "Set user priority (lower)");
	// Move ourselves after proc3 in the ready queue
	TEST_EXPECT(0, set_process_priority(PROC2_PID, LOWEST));

	test_transition("Resource contention (1 and 3 blocked)", "Resource contention (3 and 1 blocked)");
	TEST_EXPECT(0, set_process_priority(PROC3_PID, HIGH));
	TEST_EXPECT(0, set_process_priority(PROC1_PID, LOWEST));
	TEST_EXPECT(0, set_process_priority(PROC2_PID, LOWEST));
	++finished_procs;
	test_mem_release();

	TEST_EXPECT(0);
}

/**
 * A second helper process.
 */
void proc3(void)
{
	while (proc3_work_remaining > 0) {
		--proc3_work_remaining;
		TEST_EXPECT(proc2_work_remaining, proc3_work_remaining);
		TEST_EXPECT(0, release_processor());
	}

	// Since proc1 was preempted, it's at the back of the ready queue
	test_transition("Set user priority (inversion)", "Set user priority (inversion 2)");
	release_processor();

	test_transition("Resource contention (1 blocked)", "Resource contention (1 and 3 blocked)");
	test_mem_request();

	test_transition("Resource contention (3 and 1 blocked)", "Resource contention (1 blocked)");
	test_mem_release();
	TEST_EXPECT(0, set_process_priority(PROC3_PID, LOWEST));

	test_transition("Resource contention (1 blocked)", "Resource contention resolved");
	++finished_procs;
	TEST_EXPECT(0, release_processor());

	TEST_EXPECT(0);
}
