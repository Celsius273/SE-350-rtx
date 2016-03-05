/**
 * @file:   k_process.c
 * @brief:  process management C file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/02/28
 * NOTE: The example code shows one way of implementing context switching.
 *       The code only has minimal sanity check. There is no stack overflow check.
 *       The implementation assumes only two simple user processes and NO HARDWARE INTERRUPTS.
 *       The purpose is to show how context switch could be done under stated assumptions.
 *       These assumptions are not true in the required RTX Project!!!
 *       If you decide to use this piece of code, you need to understand the assumptions and
 *       the limitations.
 *@Modified: Pushpak Kumar 2016/02/27
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"
#include "sys_proc.h"
#include "list.h"
// for NULL_PRIO
#include "rtx.h"
#include <assert.h>
#include "priority_queue.h"
#include "message_queue.h"
#include "k_memory.h"
#include "timer.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
static PCB process[NUM_PROCS];   /* array of processes */
const static pid_t PID_NONE = -1;
static pid_t running = PID_NONE; /* always point to the current RUN process */

// Array of blocked PIDs
LL_DECLARE(static blocked[NUM_PROC_STATES][NUM_PRIORITIES], pid_t, NUM_PROCS);

/* array of list of processes that are in BLOCKED_ON_RESOURCE state, one for each priority */
#define g_blocked_on_resource_queue (blocked[BLOCKED_ON_RESOURCE])

/* array of list of processes that are in RDY state, one for each priority */
#define g_ready_queue (blocked[RDY])

/* array of message queues (mailbox) for each processes */
LL_DECLARE(static g_message_queues[NUM_PROCS], MSG_BUF *, NUM_MEM_BLOCKS);

/* delayed queue for messages */
static message_queue_t g_delayed_msg_queue = NULL;


static void infinite_loop(void)
{
	for (;;) {
	}
}

/* process initialization table */
const static PROC_INIT g_proc_table[] = {
	// m_pid           m_priority      m_stack_size  mpf_start_pc
	{PID_NULL,         NULL_PRIO,      0x100,        &infinite_loop},
	{PID_CLOCK,        HIGHEST,        0x100,        &proc_clock},
	// TODO add these
	{PID_KCD,          NULL_PRIO,      0x100,        &infinite_loop},
	{PID_CRT,          NULL_PRIO,      0x100,        &infinite_loop},
	{PID_TIMER_IPROC,  NULL_PRIO,      0x100,        &infinite_loop},
	{PID_UART_IPROC,   NULL_PRIO,      0x100,        &infinite_loop},
};
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];


static void initialize_processes(const PROC_INIT *const inits, int num) {
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( int i = 0; i < num; i++ ) {
		int j;
		const PROC_INIT *const init = &inits[i];
		int pid = init->m_pid;
		// Check to make sure we're not overwriting a process
		assert(pid < NUM_PROCS);
		assert(init->m_stack_size);
		assert(!process[pid].mp_sp);
		process[pid].m_pid = pid;
		process[pid].m_state = NEW;
		process[pid].m_priority = init->m_priority;
	
		// Push processes onto ready queue
		push_process(g_ready_queue, pid, process[pid].m_priority);
	
		// Initializing stack pointer for each pcb
		U32 *sp = alloc_stack(init->m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR
		*(--sp)  = (U32)(init->mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
	
		assert(sp);
		process[pid].mp_sp = sp;
	}
}

/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init()
{
  /* fill out the test initialization table */
	set_test_procs();

	// Make sure we have the right number of non-test processes
	// Fix this in k_process.h
	initialize_processes(g_proc_table, sizeof(g_proc_table) / sizeof(g_proc_table[0]));
	initialize_processes(g_test_procs, NUM_TEST_PROCS);
	// Make sure we got all the PIDs, or fill them with null
	for (int i = 0; i < NUM_PROCS; ++i) {
		if (!process[i].mp_sp) {
			process[i] = process[PID_NULL];
			process[i].m_pid = i;
		}
	}
}

/*@brief: scheduler, pick the pid of the next to run process
 *POST: 0 <= running && running < NUM_PROCS
 */

static void scheduler(void)
{
		int peek_priority = NUM_PRIORITIES;
		int peek_pid = peek_front(g_ready_queue);
		if (peek_pid != -1) {
			peek_priority = process[peek_pid].m_priority;
		}
	
		if(running != PID_NONE && peek_priority > process[running].m_priority &&
				(process[running].m_state != BLOCKED_ON_RESOURCE && process[running].m_state != BLOCKED_ON_RECEIVE)) {
			return;
		}
		
		int pid = pop_first_process(g_ready_queue);
		
		if(pid == -1) {
			running = PID_NULL;
			return;
		}

		running = pid;
}

/*@brief: switch out old process (old_pid), run the new pcb (running)
 *@param: old_pid, the pid of the old process that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  old_pid and running are valid pids.
 */
static int process_switch(pid_t old_pid)
{
	const PROC_STATE_E state = process[running].m_state;

	if (state == NEW) {
		if (old_pid != PID_NONE && running != old_pid) {
			PCB *const p_pcb_old = &process[old_pid];
			assert(p_pcb_old->m_state != NEW);
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		process[running].m_state = RUN;
		__set_MSP((U32) process[running].mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	}

	/* The following will only execute if the if block above is FALSE */

	if (running != old_pid) {
		if (state == RDY) {
			if (old_pid != PID_NONE) {
				PCB *const p_pcb_old = &process[old_pid];
				assert(p_pcb_old->m_state != NEW);
				p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			}
			process[running].m_state = RUN;
			__set_MSP((U32) process[running].mp_sp); //switch to the new proc's stack
		} else {
			running = old_pid; // revert back to the old proc on error
			assert(false);
			return RTX_ERR;
		}
	}
	return RTX_OK;
}

/**
 * @brief release_processor().
 * @return RTX_ERR on error and zero on success
 * POST: running gets updated to next to run process
 */
int k_release_processor(void)
{
	pid_t old_pid = running;
	scheduler();

	if (running == old_pid) {
		return RTX_OK;
	}
	/*
		what if current state is running? and change to blocked?

		but this changes a ton of things, breaks state assumptions
		e.g. can't assume cur proc. is running in scheduler

		before: ready queue,
		after:  blocked queue

		svc indirect calls release processor, user space calls it

	*/

	if (old_pid != PID_NONE) {
		PCB *const p_pcb_old = &process[old_pid];
		switch (p_pcb_old->m_state) {
			case BLOCKED_ON_RESOURCE:
				push_process(g_blocked_on_resource_queue, p_pcb_old->m_pid, p_pcb_old->m_priority);
				break;
			case BLOCKED_ON_RECEIVE:
				break;
			case RUN:
				p_pcb_old->m_state = RDY;
			case RDY: // fall-through
				push_process(g_ready_queue, p_pcb_old->m_pid, p_pcb_old->m_priority);
				break;
			default:
				assert(false);
		}
	}

	process_switch(old_pid);

	return RTX_OK;
}

int k_set_process_priority(int process_id, int priority) {
	// TODO check if this is correct, according to the spec.
	// "The priority of the null process may not be changed from level 4"
	if (process_id == PID_NULL && priority == NULL_PRIO) {
		return RTX_OK;
	}
	// Check for invalid values
	if(process_id < 1 || process_id >= NUM_PROCS || priority < 0 || priority >= NULL_PRIO) {
		return RTX_ERR;
	}


	PCB *p_pcb = &process[process_id];

	// If priority is the same already, then just return
	if(p_pcb->m_priority == priority) {
		return RTX_OK;
	}

	change_priority(g_ready_queue, process_id, p_pcb->m_priority, priority);
	change_priority(g_blocked_on_resource_queue, process_id, p_pcb->m_priority, priority);

	p_pcb->m_priority = priority;

	k_check_preemption();

	return RTX_OK;
}

int k_get_process_priority(int process_id) {

	// Check for invalid pid values
	if(process_id < PID_NULL || process_id >= NUM_PROCS) {
		return RTX_ERR;
	}

	// Get the pcb from the pid
	PCB *p_pcb = &process[process_id];

	return p_pcb->m_priority;
}

static void k_check_preemption_impl(bool is_eager) {
	if (k_memory_heap_free_blocks() > 0) {
		copy_queue(g_blocked_on_resource_queue, g_ready_queue);
		
		for(int i = 0; i < NUM_PROCS; ++i) {
			PCB* pcb = &process[i];
			if(pcb->m_state == BLOCKED_ON_RESOURCE) {
				pcb->m_state = RDY;
			}
		}
	}

	pid_t ready = peek_front(g_ready_queue);

	if(ready != PID_NONE) {
		// We know this won't wrap
		assert(running != PID_NONE);
		const int delta = process[running].m_priority - (int) process[ready].m_priority;
		if (is_eager ? delta >= 0 : delta > 0){
    	k_release_processor();
		}
	}
}

void k_check_preemption(void) {
	k_check_preemption_impl(false);
}

void k_check_preemption_eager(void) {
	static int millis = 0;
	millis = (millis + 1) % 100;
	if (millis == 0) {
		k_check_preemption_impl(true);
	}
}

void k_poll(PROC_STATE_E which) {
	assert(running != PID_NONE);
	PCB *const p_pcb = &process[running];
	p_pcb->m_state = which;
	switch (which) {
		case RDY:
			break;
		case BLOCKED_ON_RESOURCE:
			// Hacked in k_release_processor
			break;
		case BLOCKED_ON_RECEIVE:
			// Also hacked in k_release_processor
			break;
		default:
			assert(false);
	}
	k_release_processor();
}

static int k_enqueue_ready_process(pid_t receiver_pid)
{
    if(receiver_pid == PID_NONE) {
        return RTX_ERR;
    }
    
    push_process(g_ready_queue, receiver_pid, process[receiver_pid].m_priority);
    
    return RTX_OK;
}

/**
 * Send a message. Must have IRQ lock.
 */
static void k_send_message_helper(int sender_pid, int receiver_pid, void *p_msg)
{
    MSG_BUF *p_msg_envelope = NULL;
    PCB *p_receiver_pcb = NULL;
	
    p_msg_envelope = (MSG_BUF *)((U8 *)p_msg);
    p_msg_envelope->m_send_pid = sender_pid;
    p_msg_envelope->m_recv_pid = receiver_pid;
    
    p_receiver_pcb = &process[receiver_pid];
    
    LL_PUSH_BACK(g_message_queues[receiver_pid], p_msg_envelope);
		
    if (p_receiver_pcb->m_state == BLOCKED_ON_RECEIVE) {
        //if the process was previously in the blocked queue, unblock it and put it in the ready queue
        p_receiver_pcb->m_state = RDY;
        k_enqueue_ready_process(receiver_pid);
    }
}

static bool validate_message(int receiver_pid, void *p_msg_env) {
	if (p_msg_env == NULL) {
		return false;
	}
    
  if (receiver_pid < 0 || receiver_pid >= NUM_PROCS) {
		return false;
	}
	return true;
}

/*Inter Process Communication Methods*/
int k_send_message(int receiver_pid, void *p_msg_env)
{
	if (!validate_message(receiver_pid, p_msg_env)) {
		return RTX_ERR;
	}

	disable_irq();
	k_send_message_helper(process[running].m_pid, receiver_pid, p_msg_env);
		//if the receiving process is of higher priority, preemption might happen
	enable_irq();

	k_check_preemption();
	
	return RTX_OK;
}

void *k_receive_message(int *p_sender_pid)
{
	MSG_BUF *p_msg = NULL;
	
	disable_irq();
	while (LL_SIZE(g_message_queues[process[running].m_pid]) == 0) {
		enable_irq();
		k_poll(BLOCKED_ON_RECEIVE);
	disable_irq();
		assert(LL_SIZE(g_message_queues[process[running].m_pid]) > 0);
	}
	
	p_msg = (MSG_BUF *)LL_POP_FRONT(g_message_queues[process[running].m_pid]);
	
	if (p_msg == NULL) {
		enable_irq();
		
			return NULL;
	}
	
	if (p_sender_pid != NULL) {
		//Note the sender_id is an output parameter and is not meant to filter which message to receive.
		*p_sender_pid = p_msg->m_send_pid;
	}
	
	enable_irq();
	
	return (void *)((U8 *)p_msg);
}

void *k_non_blocking_receive_message(int pid)
{
		disable_irq();

    if (!(LL_SIZE(g_message_queues[pid]) == 0)) {
        MSG_BUF *p_msg = (MSG_BUF *)LL_POP_FRONT(g_message_queues[pid]);
        enable_irq();
				return (void *)((U8 *)p_msg);
    } else {
				enable_irq();
        return NULL;
    }
}

int k_delayed_send(int receiver_id, void *p_msg_env, int delay) {
	if (!validate_message(receiver_id, p_msg_env)) {
		return RTX_ERR;
	}
	
	// If the delay is zero, that means we do not delay, and we just send the message right away
	if (delay == 0) {
		return k_send_message(receiver_id, p_msg_env);
	}
	
		MSG_BUF *const p_msg_envelope = p_msg_env;

		assert(running != PID_NONE);
    p_msg_envelope->m_send_pid = process[running].m_pid;
    p_msg_envelope->m_recv_pid = receiver_id;
		p_msg_envelope->m_kdata[0] = delay + g_timer_count;

	 // insert new message into sorted queue (in desc. order of expiry time)
    enqueue_message(p_msg_envelope, &g_delayed_msg_queue);
	
		return RTX_OK;
}

void check_delayed_messages(void) {
	disable_irq();
	for (;;) {
		MSG_BUF *const msg = dequeue_message(&g_delayed_msg_queue);
		if (!msg) {
			break;
		}
		k_send_message_helper(msg->m_send_pid, msg->m_recv_pid, msg);
	}
	enable_irq();
}

// Allow recursive IRQ disable

static int irq_lock_count = 0;

void enable_irq(void) {
	assert(irq_lock_count > 0);
	if (--irq_lock_count == 0) {
		__enable_irq();
	}
}

void disable_irq(void) {
	__disable_irq();
	++irq_lock_count;
}
