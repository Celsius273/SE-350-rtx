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
 *@Modified: Pushpak Kumar 2016/01/27
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

#ifdef DEBUG_0
#include "printf.h"
#endif /* DEBUG_0 */

/* ----- Global Variables ----- */
PCB *gp_pcbs[NUM_TEST_PROCS];   /* array of pcbs pointers */
PCB *gp_current_process = NULL; /* always point to the current RUN process */
U32 g_switch_flag = 0;          /* whether to continue to run the process before the UART receive interrupt */
                                /* 1 means to switch to another process, 0 means to continue the current process */
																/* this value will be set by UART handler */

/* array of list of processes that are in BLOCKED_ON_RESOURCE state, one for each priority */
ll_header_t g_blocked_on_resource_queue[NUM_PRIORITIES];

/* array of list of processes that are in READY state, one for each priority */
ll_header_t g_ready_queue[NUM_PRIORITIES];

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
	for ( i = 0; i < NUM_TEST_PROCS; i++ ) {
		g_proc_table[num_procs++] = g_test_procs[i];
	}
	g_proc_table[num_procs++] = (PROC_INIT) {
		.m_pid = NULL_PID,
		.m_priority = NULL_PRIO,
		.m_stack_size = 0x100,
		.mpf_start_pc = &infinite_loop,
	};
  
	assert(NUM_PROCS == num_procs);
	/* initilize exception stack frame (i.e. initial context) for each process */
	for ( i = 0; i < NUM_PROCS; i++ ) {
		int j;
		(gp_pcbs[i])->m_pid = (g_proc_table[i]).m_pid;
		(gp_pcbs[i])->m_state = NEW;
		
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
	if (gp_current_process == NULL) {
		gp_current_process = gp_pcbs[0];
		return gp_pcbs[0];
	}

	if ( gp_current_process == gp_pcbs[0] ) {
		return gp_pcbs[1];
	} else if ( gp_current_process == gp_pcbs[1] ) {
		return gp_pcbs[0];
	} else {
		return NULL;
	}
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
//	PCB *p_pcb_old = NULL;
//
//	p_pcb_old = gp_current_process;
//	gp_current_process = scheduler();
//
//	if ( gp_current_process == NULL  ) {
//		gp_current_process = p_pcb_old; // revert back to the old process
//		return RTX_ERR;
//	}
//        if ( p_pcb_old == NULL ) {
//		p_pcb_old = gp_current_process;
//	}
//	process_switch(p_pcb_old);
//	return RTX_OK;
	return RTX_ERR;
}

/*set the state of the p_pcb to BLOCKED_ON_RESOURCE and enqueue it in the blocked_on_resource queue*/
void k_enqueue_blocked_on_resource_process(PCB* p_pcb)
{
	/*queue_t can be implement using the generic linked list*/
	LL_DECLARE(p_blocked_on_resource_queue, PCB*, 6);
	if(p_pcb == NULL){
		return;
	}
	p_pcb->m_state = BLOCKED_ON_RESOURCE;
	p_blocked_on_resource_queue = &g_blocked_on_resource_queue[p_pcb->m_priority];

	//Need a mechanism to check if a node already exists in the queue
	/*
	if(!(LL_SIZE(p_blocked_on_resource_queue)==0) && queue_contains_node(p_blocked_on_resource_queue, (node_t*)p_pcb)) {
		return;
	}
	*/
	LL_PUSH_BACK(p_pcb, p_blocked_on_resource_queue);
}

/*dequeue the next available process in blocked_on_resource queue*/
PCB* k_dequeue_blocked_on_resource_process(void)
{
	PCB *p_pcb = NULL;
	for(int i = 0; i < NUM_PRIORITIES; i++) {
		if(!(LL_SIZE(g_blocked_on_resource_queue[i]) == 0)){
			p_pcb = (PCB *)LL_POP_FRONT(g_blocked_on_memory_queue[i]);
			break;
		}
	}
	return p_pcb;
}

void k_enqueue_ready_process(PCB *p_pcb)
{
	/* enqueue the PCB in the ready queue corresponding to its priority */
    LL_PUSH_BACK(p_pcb, &g_ready_queue[p_pcb->m_priority]);

}
