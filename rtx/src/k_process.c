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
 *@Modified: Jobair Hassan, Kelvin Jiang 2016/02/04
 */

#include <LPC17xx.h>
#include <system_LPC17xx.h>
#include "uart_polling.h"
#include "k_process.h"
// FIXME are awe allowed to refer to user code from kernel?
#include "usr_proc.h"
// for NULL_PRIO
#include "rtx.h"
#include <assert.h>
#include "priority_queue.h"

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */


/* ----- Global Variables ----- */
PCB **gp_pcbs;   /* array of pcbs pointers */
PCB *gp_current_process = NULL; /* always point to the current RUN process */
U32 g_switch_flag = 0;          /* whether to continue to run the process before the UART receive interrupt */
                                /* 1 means to switch to another process, 0 means to continue the current process */
																/* this value will be set by UART handler */

/* array of list of processes that are in BLOCKED_ON_RESOURCE state, one for each priority */
LL_DECLARE(g_blocked_on_resource_queue[NUM_PRIORITIES], pid_t, NUM_PROCS);

/* array of list of processes that are in READY state, one for each priority */
LL_DECLARE(g_ready_queue[NUM_PRIORITIES], pid_t, NUM_PROCS);

/* process initialization table */
PROC_INIT g_proc_table[NUM_PROCS];
extern PROC_INIT g_test_procs[NUM_TEST_PROCS];

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
		.m_pid = NULL_PID,
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
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_state = NEW;
		(gp_pcbs[i])->m_priority = (g_proc_table[i]).m_priority;
	
		// Push processes onto ready queue
		push_process(g_ready_queue, gp_pcbs[i]->m_pid, gp_pcbs[i]->m_priority);
	
		// Initializing stack pointer for each pcb
		sp = alloc_stack((g_proc_table[i]).m_stack_size);
		*(--sp)  = INITIAL_xPSR;      // user process initial xPSR  
		*(--sp)  = (U32)((g_proc_table[i]).mpf_start_pc); // PC contains the entry point of the process
		for ( j = 0; j < 6; j++ ) { // R0-R3, R12 are cleared with 0
			*(--sp) = 0x0;
		}
		
		(gp_pcbs[i])->mp_sp = sp;
	}
}

/*@brief: scheduler, pick the pid of the next to run process
 *@return: PCB pointer of the next to run process
 *         NULL if error happens
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */

PCB *scheduler(void)
{
 	  int pid = pop_first_process(g_ready_queue);

		if(pid == -1) {
			return gp_pcbs[NULL_PID];
		}
		
		if(NULL == gp_current_process) {
			gp_current_process = gp_pcbs[NULL_PID];
		}
		
		return gp_pcbs[pid];
}

/*@brief: switch out old pcb (p_pcb_old), run the new pcb (gp_current_process)
 *@param: p_pcb_old, the old pcb that was in RUN
 *@return: RTX_OK upon success
 *         RTX_ERR upon failure
 *PRE:  p_pcb_old and gp_current_process are pointing to valid PCBs.
 *POST: if gp_current_process was NULL, then it gets set to pcbs[0].
 *      No other effect on other global variables.
 */
int process_switch(PCB *p_pcb_old)
{
	PROC_STATE_E state;

	state = gp_current_process->m_state;

	if (state == NEW) {
		if (gp_current_process != p_pcb_old && p_pcb_old->m_state != NEW) {
			p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP();
		}
		gp_current_process->m_state = RUN;
		__set_MSP((U32) gp_current_process->mp_sp);
		__rte();  // pop exception stack frame from the stack for a new processes
	}

	/* The following will only execute if the if block above is FALSE */

	if (gp_current_process != p_pcb_old) {
		if (state == RDY){
			p_pcb_old->m_state = RDY;
			p_pcb_old->mp_sp = (U32 *) __get_MSP(); // save the old process's sp
			gp_current_process->m_state = RUN;
			__set_MSP((U32) gp_current_process->mp_sp); //switch to the new proc's stack
		} else {
			gp_current_process = p_pcb_old; // revert back to the old proc on error
			return RTX_ERR;
		}
	}
	return RTX_OK;
}

/**
 * @brief release_processor().
 * @return RTX_ERR on error and zero on success
 * POST: gp_current_process gets updated to next to run process
 */
int k_release_processor(void)
{
	PCB *p_pcb_old = NULL;

	p_pcb_old = gp_current_process;
	gp_current_process = scheduler();
	
	if ( gp_current_process == NULL && p_pcb_old == gp_pcbs[0]) {
		gp_current_process = p_pcb_old; // revert back to the null process
		return RTX_OK;
	}

	push_process(g_ready_queue, p_pcb_old->m_pid, p_pcb_old->m_priority);
	process_switch(p_pcb_old);
		
	return RTX_OK;
	
}

/*set the state of the p_pcb to BLOCKED_ON_RESOURCE and enqueue it in the blocked_on_resource queue*/
int k_enqueue_blocked_on_resource_process(PCB* p_pcb)
{
	// Error checking	
	if(p_pcb == NULL){
		return RTX_ERR;
	}
	
	// Change the state of the process to blocked on resource
	p_pcb->m_state = BLOCKED_ON_RESOURCE;

	// Remove from ready queue, and put it into blocked queue, so we call move_process (which deques from
	// ready queue, then pushes onto blocked queue
	move_process(g_ready_queue, g_blocked_on_resource_queue, p_pcb->m_pid);
		
	return RTX_OK;
}

/*dequeue the next available process in blocked_on_resource queue*/
PCB* k_dequeue_blocked_on_resource_process(void)
{
	int pid = pop_first_process(g_blocked_on_resource_queue);
	
	if(pid == -1) {
		return NULL;
	}
	
	return gp_pcbs[pid];
}

int k_enqueue_ready_process(PCB *p_pcb)
{
	if(NULL == p_pcb) {
		return RTX_ERR; 
	}
	
	push_process(g_ready_queue, p_pcb->m_pid, p_pcb->m_priority);
	
	return RTX_OK;
}

//TODO: Do we need this?

// PCB* k_dequeue_ready_process(void)
// {	
// 	return gp_pcbs[pop_first_process(g_ready_queue)];
// 	
// }


	

int set_process_priority(int process_id, int priority) {
	// TODO check if this is correct, according to the spec.
	// "The priority of the null process may not be changed from level 4"
	if (process_id == NULL_PID && priority == NULL_PRIO) {
		return RTX_OK;
	}
	// Check for invalid values
	if(process_id < 1 || process_id >= NUM_PROCS || priority < 0 || priority >= NULL_PRIO) {
		return RTX_ERR;
	}
	

	PCB *p_pcb = NULL;
	p_pcb = gp_pcbs[process_id];
	
	// If priority is the same already, then just return
	if(p_pcb->m_priority == priority) {
		return RTX_OK;
	}
	
	change_priority(g_ready_queue, process_id, p_pcb->m_priority, priority);
	change_priority(g_blocked_on_resource_queue, process_id, p_pcb->m_priority, priority);
	
	p_pcb->m_priority = priority;
	
	k_release_processor();
	
	return RTX_OK;
}

int get_process_priority(int process_id) {
	
	// Check for invalid pid values
	if(process_id < NULL_PID || process_id >= NUM_PROCS) {
		return RTX_ERR;
	}
	
	// Get the pcb from the pid
	PCB *p_pcb = gp_pcbs[process_id];
	
	return p_pcb->m_priority;
}
