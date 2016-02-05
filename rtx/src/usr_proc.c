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
#include <assert.h>

// TODO set these constants
#define NUM_TESTS 123
#define GROUP_ID "004"

#define test_printf(...) printf("G" GROUP_ID "_test: " __VA_ARGS__)

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];
static volatile int proc2_work_remaining = 3, proc3_work_remaining = 3, finished_proc = 0;
static int tests_ran = 0, tests_failed = 0;
const char *test_state = "Starting tests";

void test_assert(int expected, const char *msg, int lineno) {
	const char *status = expected ? "OK" : "FAIL";
	if (!expected) {
		++tests_failed;
		#ifdef DEBUG_0
			printf("Assertion failed in %s:%d: %s\n", __FILE__, lineno, msg);
		#endif
	}
	test_printf("test %d %s\n", ++tests_ran, status);
}

#define TEST_ASSERT(expected) \
	test_assert(!!(expected), #expected, __LINE__); \

#define TEST_EXPECT(expected, actual) TEST_ASSERT((expected) == (actual));

static int test_release_processor_impl(const char *procname) {
	printf("Unscheduling %s\n", procname);
	int ret = release_processor();
	printf("Rescheduling %s\n", procname);
	return ret;
}
#define test_release_processor() test_release_processor_impl(__FUNCTION__)

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=USR_SZ_STACK;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
	g_test_procs[2].mpf_start_pc = &proc3;
}

static void test_transition_impl(const char *from, const char *to, int lineno)
{
#ifdef DEBUG_0
	if (from != test_state) {
		printf(
			"test_transition(%s, %s) expected state %s, but got %s\n",
			from,
			to,
			from,
			test_state
		);
	}
#endif
	test_assert(from == test_state, "from == test_state (OS scheduled wrong process)", lineno);
	printf("Done: %s, starting %s\n", test_state, to);
	test_state = to;
}
#define test_transition(from, to) test_transition_impl((from), (to), __LINE__)

void infinite_loop(void)
{
	for (;;) {
		test_release_processor();
	}
}

typedef struct mem_block {
	struct mem_block *volatile next;
	volatile char data[128 - sizeof(struct mem_block *)];
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

	printf("Releasing memory block 0x%08x\n", (unsigned long) cur);
	TEST_EXPECT(0, release_memory_block(cur));
}

void *test_mem_request(void) {
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

#define PROC1_PID 1
#define PROC2_PID 2
#define PROC3_PID 3
#define PROC4_PID 4
#define PROC5_PID 5
#define PROC6_PID 6

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
	test_transition(test_state, "FIFO scheduling");
	assert(proc2_work_remaining == 3);
	for (int i = 2; i >= 0; --i) {
		TEST_EXPECT(0, test_release_processor());
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
	TEST_EXPECT(0, test_release_processor());

	test_transition("Release processor (max priority)", "Resource contention (1 blocked)");
	test_mem_request();

	test_transition("Resource contention resolved", "Count blocks");
	int blocks = 0;
	while (test_mem_front != NULL) {
		test_mem_release();
		++blocks;
	}
	TEST_ASSERT(blocks >= 30);
	test_transition("Count blocks", "Test finished");

	TEST_EXPECT(0, changed_bytes);
	TEST_ASSERT(finished_proc >= 2);
	// Wait for proc{4,5,6} to "finish"
	while (finished_proc < 5) {
		test_release_processor();
	}

	test_printf("%d/%d tests OK\n", tests_ran - tests_failed, tests_ran);
	test_printf("%d/%d tests FAIL\n", tests_failed, tests_ran);
	// This invalidates tests_ran, but since all processes have reached
	//   infinite_loop(), everything is okay.
	TEST_EXPECT(tests_ran, NUM_TESTS);
	test_printf("END\n");
	infinite_loop();
}

/**
 * @brief: a helper process to proc1
 */
void proc2(void)
{
	printf("Started %s\n", __FUNCTION__);
	while (proc2_work_remaining > 0) {
		TEST_EXPECT(proc3_work_remaining, proc2_work_remaining);
		--proc2_work_remaining;
		TEST_EXPECT(0, test_release_processor());
	}

	test_transition("Equal priority memory blocking", "Equal priority memory unblocking");
	test_mem_release();

	test_transition("Equal priority memory unblocking", "Equal priority memory unblocked");
	// Should schedule proc3 since proc1 was preempted recently
	TEST_EXPECT(0, test_release_processor());

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
	++finished_proc;
	test_mem_release();

	TEST_ASSERT(0);
}

/**
 * A second helper process.
 */
void proc3(void)
{
	printf("Started %s\n", __FUNCTION__);
	while (proc3_work_remaining > 0) {
		--proc3_work_remaining;
		TEST_EXPECT(proc2_work_remaining, proc3_work_remaining);
		TEST_EXPECT(0, test_release_processor());
	}

	// Since proc1 was preempted, it's at the back of the ready queue
	test_transition("Set user priority (inversion)", "Set user priority (inversion 2)");
	test_release_processor();

	test_transition("Resource contention (1 blocked)", "Resource contention (1 and 3 blocked)");
	test_mem_request();

	test_transition("Resource contention (3 and 1 blocked)", "Resource contention (1 blocked)");
	test_mem_release();
	TEST_EXPECT(0, set_process_priority(PROC3_PID, LOWEST));

	test_transition("Resource contention (1 blocked)", "Resource contention resolved");
	++finished_proc;
	TEST_EXPECT(0, test_release_processor());

	TEST_ASSERT(0);
}

/**
 * A test process that theoretically should not affect proc1-proc3.
 * It does calculations and changes its own priority.
 */
void proc4(void)
{
	for (int iteration = 0;; test_release_processor()) {
		if (iteration < 3) {
			++iteration;
		} else {
			// We want this process to run at least 3 iterations, before it's "finished".
			++finished_proc;
		}

		{
			// Cycle the priority
			const int prio = get_process_priority(PROC4_PID);
			TEST_EXPECT(RTX_OK, set_process_priority(PROC4_PID, (prio + 1) % NULL_PRIO));
		}

		printf("Doing computations that shouldn't affect other processes\n");

		{
			// Check the buffer allows word access
			int *buf = request_memory_block();
			for (int i = 0; i < 32; ++i) {
				buf[i] = -1;
			}
			for (int i = 0; i < 32; ++i) {
				buf[((7 * i) % 32 + 32) % 32] = i;
			}
			TEST_EXPECT(-1, buf[4]);
			TEST_EXPECT(23, buf[7]);
			release_memory_block(buf);
		}

		{
			double *buf = request_memory_block();
			// These values should be stored correctly
			buf[0] = 1.5;
			buf[1] = 3. / 2;
			TEST_EXPECT(1., buf[0] / buf[1]);
			release_memory_block(buf);
		}
	}
}

// Test stack usage and shared memory works.
// These processes will use a lot of stack memory and shared memory.
// They implement quicksort.

#define SORT_SIZE 10
static volatile unsigned int sort_numbers[SORT_SIZE];
static volatile int sort_lo = 0, sort_hi = 0;

static void sort_reset() {
	int is_sorted = 1;
	for (int i = 1; i < SORT_SIZE; ++i) {
		is_sorted = is_sorted && sort_numbers[i-1] > sort_numbers[i];
	}
	TEST_ASSERT(is_sorted);

	static unsigned int random = 2429631604U;
	for (int i = 0; i < SORT_SIZE; ++i) {
		sort_numbers[i] = random % 3 + 2;
		random = random * 1664525 + 1013904223;
	}
}

/**
 * Recursively quicksort.
 * This uses a lot of stack.
 */
static void sort_quicksort(int lo, int hi) {
	if (hi - lo < 1) {
		return;
	}

	sort_lo = lo;
	sort_hi = hi;
	// Wait for proc6 to finish partitioning
	while (sort_lo < sort_hi) {
		test_release_processor();
	}

	int pivot = sort_lo;
	sort_quicksort(lo, pivot);
	sort_quicksort(pivot + 1, hi);
}

/**
 * This process is responsible for quicksort recursion.
 * This process works with proc6 to test that the user stack is correctly allocated.
 * These two processes use a bunch of stack space and communicate with each other.
 */
void proc5(void)
{
	for (int iteration = 0;; test_release_processor()) {
		if (iteration < 3) {
			++iteration;
		} else {
			// We want this process to run at least 3 iterations, before it's "finished".
			++finished_proc;
		}

		sort_reset();
		sort_quicksort(0, SORT_SIZE);
	}
}

static int sort_partition(volatile unsigned int *arr, int len) {
	int lo_vals[SORT_SIZE], hi_vals[SORT_SIZE];
	int num_lo = 0, num_hi = 0;
	int pivot = arr[0];
	for (int i = 0; i < len; ++i) {
		if (arr[i] < pivot) lo_vals[num_lo++] = arr[i];
		else hi_vals[num_hi++] = arr[i];
	}
	for (int i = 0; i < num_lo; ++i) arr[i] = lo_vals[i];
	for (int i = 0; i < num_hi; ++i) arr[num_lo + i] = hi_vals[i];
	return num_lo - 1;
}

/**
 * This process is responsible for quicksort partitioning.
 * This process works with proc5 to test that the user stack is correctly allocated.
 * These two processes use a bunch of stack space and communicate with each other.
 */
void proc6(void)
{

	for (int iteration = 0;; test_release_processor()) {
		if (iteration < 3) {
			++iteration;
		} else {
			// We want this process to run at least 3 iterations, before it's "finished".
			++finished_proc;
		}

		if (sort_lo < sort_hi) {
			sort_lo = sort_hi = sort_partition(&sort_numbers[sort_lo], sort_hi - sort_lo);
		}
	}
}
