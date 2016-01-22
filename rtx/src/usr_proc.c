/**
 * @file:   usr_proc.c
 * @brief:  Two user processes: proc1 and proc2
 * @author: Yiqing Huang
 * @date:   2014/02/28
 * NOTE: Each process is in an infinite loop. Processes never terminate.
 */

#include "rtx.h"
#include "uart_polling.h"
#include "usr_proc.h"
#include "printf.h"

// TODO set group id
#define test_printf(...) printf("Gid_test: " __VA_ARGS__)

/* initialization table item */
PROC_INIT g_test_procs[NUM_TEST_PROCS];
static int proc2_work_remaining = 3;
static int tests_run = 0, tests_failed = 0;
const char *state = "initial";
#define TOTAL_TESTS 10

void test_assert(int expected, const char *msg, int lineno) {
	const char *status = expected ? "OK" : "FAIL";
	if (!expected) {
		++tests_failed;
	}
	test_printf("test %d %s\n", ++tests_run, status);
#ifdef DEBUG_0
	printf("Assertion failed in %s:%d: %s\n", __FILE__, lineno, msg);
#endif
}

#define TEST_ASSERT(expected) \
	test_assert(!!(expected), #expected, __LINE__); \

#define TEST_EXPECT(expected, actual) TEST_ASSERT((actual) == (expected));

void set_test_procs() {
	int i;
	for( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_test_procs[i].m_pid=(U32)(i+1);
		g_test_procs[i].m_priority=LOWEST;
		g_test_procs[i].m_stack_size=0x100;
	}
  
	g_test_procs[0].mpf_start_pc = &proc1;
	g_test_procs[1].mpf_start_pc = &proc2;
}

static void state_transition(const char *from, const char *to)
{
	TEST_EXPECT(state == from);
	state = to;
}

static void infinite_loop(void)
{
	for (;;) {
		release_processor();
	}
}

#define PROC1_PID 0
#define PROC2_PID 1
/**
 * @brief: a process that tests the RTX API
 */
void proc1(void)
{
	// int release_processor();
	// This primitive transfers the control to the RTX (the calling process voluntarily releases
	// the processor). The invoking process remains ready to execute and is put at the end of the
	// ready queue of the same priority. Another process may possibly be selected for execution.
	state_transition("initial", "FIFO scheduling")
	assert(proc2_work_remaining == 3);
	for (int i = 2; i >= 0; --i) {
		release_processor();
		assert(proc2_work_remaining == i);
	}

	state_transition("FIFO scheduling", "...");
	;

	infinite_loop();
}

/**
 * @brief: a helper process to proc1
 */
void proc2(void)
{
	while (proc2_work_remaining > 0) {
		--proc2_work_remaining;
		release_processor();
	}

	infinite_loop();
}
