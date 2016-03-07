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
#include "common.h"
#include "k_process.h"
#include "usr_proc.h"
#include "printf.h"

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

#define MIN_MEM_BLOCKS 5

/**
 * @brief: a process that tests the RTX API
 */
void proc1(void)
{
	infinite_loop();
}

/**
 * @brief: a helper process to proc1
 */
void proc2(void)
{
	infinite_loop();
}

/**
 * A second helper process.
 */
void proc3(void)
{
	infinite_loop();
}

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

// Process to show blocked on receive state hotkey
void proc4(void)
{
	/*
	for (;;) {
		receive_message(NULL);
	}
	
	++finished_proc;
	*/
	test_transition(test_state, "Setup set priorities");
	test_set_process_priority(PID_P4, HIGH);
	test_set_process_priority(PID_P5, HIGH);
	
	int ia = 0;
	for (;;) {
		crt_printf("PROC 4 OH YES! PRINT NO:%d \n", ia++);
	}
	infinite_loop();
}

// Process to show ready state hotkey
void proc5(void)
{	
	for (;;) {
		crt_printf("proc5 steals dah show!\n");
	}
	infinite_loop();
}

// Process to show blocked on memory hotkey
void proc6(void)
{
	for (;;) {
		release_memory_block(request_memory_block());
		release_processor();
	}
}
