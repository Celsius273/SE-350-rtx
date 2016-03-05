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
#include "list.h"
// for NULL_PRIO
#include "rtx.h"
#include <assert.h>
#include "priority_queue.h"
#include "message_queue.h"
#include "k_memory.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB process[NUM_PROCS];   /* array of processes */
static pid_t running = -1; /* always point to the current RUN process */

// Array of blocked PIDs
LL_DECLARE(static blocked[NUM_PROC_STATES][NUM_PRIORITIES], pid_t, NUM_PROCS);

/* array of list of processes that are in BLOCKED_ON_RESOURCE state, one for each priority */
#define g_blocked_on_resource_queue (blocked[BLOCKED_ON_RESOURCE])

/* array of list of processes that are in RDY state, one for each priority */
#define g_ready_queue (blocked[RDY])

/* array of list of processes that are in BLOCKED_ON_RECEIVE state, one for each priority */
#define g_blocked_on_receive_queue (blocked[BLOCKED_ON_RECEIVE])

/* array of message queues (mailbox) for each processes */
LL_DECLARE(static g_message_queues[NUM_PROCS], MSG_BUF *, NUM_MEM_BLOCKS);

/* delayed queue for messages */
static message_queue_t g_delayed_msg_queue = NULL;


/* process initialization table */
PROC_INIT g_proc_table[NUM_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

static void infinite_loop(void)
{
	for (;;) {
	}
}

/**
 * @biref: initialize all processes in the system
 * NOTE: We assume there are only two user processes in the system in this example.
 */
void process_init()
{
	int i;
	U32 *sp;

  /* fill out the initialization table */
	set_test_procs();
	int num_procs = 0;

	// This is for the null process
	g_proc_table[num_procs++] = (PROC_INIT) {
		.m_pid = PID_NULL,
		.m_priority = NULL_PRIO,
		.m_stack_size = 0x100,
		.mpf_start_pc = &infinite_loop,
	};

	// Setup initialization for all user processes
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[num_procs++] = g_test_procs[i];
	}

	assert(NUM_PROCS == num_procs);

	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_PROCS; i++ ) {
		int j;
		process[i].m_pid = (g_proc_table[i]).m_pid;
		process[i].m_state = NEW;
		process[i].m_priority = (g_proc_table[i]).m_priority;

		// Push processes onto ready queue
		push_process(g_ready_queue, process[i].m_pid, process[i].m_priority);

		// Initializing stack pointer for each pcb
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}

		process[i].mp_sp = sp;
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
	
		if(running != -1 && peek_priority > process[running].m_priority && process[running].m_state != BLOCKED_ON_RESOURCE) {
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
	PCB *const p_pcb_old = &process[old_pid];

	if (state == NEW) {
		if (running != old_pid && p_pcb_old->m_state != NEW) {
			p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		process[running].m_state = RUN;
		__set_MSP((U32) process[running].mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	}

	/* The following will only execute if the if block above is FALSE */

	if (running != old_pid) {
		if (state == RDY){
			p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			process[running].m_state = RUN;
			__set_MSP((U32) process[running].mp_sp); //switch to the new proc's stack
		} else {
			running = old_pid; // revert back to the old proc on error
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
	PCB *const p_pcb_old = &process[old_pid];
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
	if (p_pcb_old->m_state == BLOCKED_ON_RESOURCE) {
		push_process(g_blocked_on_resource_queue, p_pcb_old->m_pid, p_pcb_old->m_priority);
	}
	else {
		push_process(g_ready_queue, p_pcb_old->m_pid, p_pcb_old->m_priority);
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

void k_check_preemption(void) {
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

	if(ready != -1 && process[running].m_priority > process[ready].m_priority){
    	k_release_processor();
    }
}


void k_poll(PROC_STATE_E which) {
	assert(running != -1);
	PCB *const p_pcb = &process[running];
	p_pcb->m_state = which;
	switch (which) {
		case RDY:
			break;
		case BLOCKED_ON_RESOURCE:
			// Hacked in k_release_processor
			break;
		case BLOCKED_ON_RECEIVE:
			{
				void *p_blocked_on_receive_queue = &g_blocked_on_receive_queue[p_pcb->m_priority];

				//don't re-add the process if it has already been added to the queue
				if (!(
							!is_pid_queue_empty(p_blocked_on_receive_queue)
							&& queue_contains_node(p_blocked_on_receive_queue, p_pcb->m_pid)
					 )) {	//Kelvin: Please add queue_contains_node(void*, pcb_id)
					/* put process in the blocked-on-receive-queue */
					push_process(g_blocked_on_receive_queue, p_pcb->m_pid, p_pcb->m_priority);
				}
			}
			break;
		default:
			assert(false);
	}
	k_release_processor();
}

static int k_enqueue_ready_process(PCB *p_pcb)
{
    if(NULL == p_pcb) {
        return RTX_ERR;
    }
    
    push_process(g_ready_queue, p_pcb->m_pid, p_pcb->m_priority);
    
    return RTX_OK;
}

static int k_send_message_helper(int sender_pid, int receiver_pid, void *p_msg)
{
    MSG_BUF *p_msg_envelope = NULL;
    PCB *p_receiver_pcb = NULL;
    
	  __disable_irq();
    p_msg_envelope = (MSG_BUF *)((U8 *)p_msg);
    p_msg_envelope->m_send_pid = sender_pid;
    p_msg_envelope->m_recv_pid = receiver_pid;
    
    p_receiver_pcb = &process[receiver_pid];
    
    LL_PUSH_BACK(g_message_queues[receiver_pid], p_msg_envelope);
		
    if (p_receiver_pcb->m_state == BLOCKED_ON_RECEIVE) {
        //if the process was previously in the blocked queue, unblock it and put it in the ready queue
        remove_from_queue(&g_blocked_on_receive_queue[p_receiver_pcb->m_priority] , p_receiver_pcb->m_pid);		//Kelvin: implement remove_from_queue
        p_receiver_pcb->m_state = RDY;
        k_enqueue_ready_process(p_receiver_pcb);
				k_check_preemption();
			  __enable_irq();
        return 1;	//signals that receiver is unblocked and put onto the ready
    } else {
			  __enable_irq();
        return 0;
    }
}


/*Inter Process Communication Methods*/
int k_send_message(int receiver_pid, void *p_msg_env)
{
	if (p_msg_env == NULL) {
		return RTX_ERR;
	}
    
  if (receiver_pid < 0 || receiver_pid >= NUM_PROCS) {
		return RTX_ERR;
	}
  
	__disable_irq();
	if (k_send_message_helper(process[running].m_pid, receiver_pid, p_msg_env) == 1) {
		//if the receiving process is of higher priority, preemption might happen
		if (process[receiver_pid].m_priority <= process[running].m_priority) {
			int ret = k_release_processor();
			__enable_irq();
			return ret;
		}
	}
	__enable_irq();
	return RTX_ERR;
}

void *k_receive_message(int *p_sender_pid)
{
	MSG_BUF *p_msg = NULL;
	
	__disable_irq();
	while (LL_SIZE(g_message_queues[process[running].m_pid]) == 0) {
		k_poll(BLOCKED_ON_RECEIVE);
	}
	
	p_msg = (MSG_BUF *)LL_POP_FRONT(g_message_queues[process[running].m_pid]);		//Kelvin: Add dequeue_message(void* pq) to your priority queue API somehow
	
	if (p_msg == NULL) {
		__enable_irq();
			return NULL;
	}
	
	if (p_sender_pid != NULL) {
		//Note the sender_id is an output parameter and is not meant to filter which message to receive.
		*p_sender_pid = p_msg->m_send_pid;
	}
	
	__enable_irq();
	return (void *)((U8 *)p_msg);
}

void *k_non_blocking_receive_message(int pid)
{
		__disable_irq();
    if (!(LL_SIZE(g_message_queues[pid]) == 0)) {
        MSG_BUF *p_msg = (MSG_BUF *)LL_POP_FRONT(g_message_queues[pid]);
        __enable_irq();
				return (void *)((U8 *)p_msg);
    } else {
				__enable_irq();
        return NULL;
    }
}

int k_delayed_send(int sender_pid, void *p_msg_env, int delay)
{
	if(sender_pid < PID_NULL || sender_pid >= NUM_PROCS || delay < 0) {
		return RTX_ERR;
	}
	
	// If the delay is zero, that means we do not delay, and we just send the message right away
	if(delay == 0) {
		return k_send_message(sender_pid, p_msg_env);
	}
	
	/*
		MSG_BUF *p_msg_envelope = NULL;
    
    p_msg_envelope = (MSG_BUF *)((U8 *)p_msg_env - MSG_HEADER_OFFSET); // Requesting without adding?? 
    p_msg_envelope->m_send_pid = sender_pid;
    p_msg_envelope->m_recv_pid = process[running].m_pid; */
	
	//p_msg_envelope->m_expiry = delay + g_timer_count;
	
	 // insert new message into sorted queue (in desc. order of expiry time)
    //enqueue_delayed_message_in_sorted_order(p_msg_envelope);
	

	return RTX_OK;
}
