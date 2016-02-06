/**
 * @file:   k_process.h
 * @brief:  process management hearder file
 * @author: Yiqing Huang
 * @author: Thomas Reidemeister
 * @date:   2014/01/17
 * NOTE: Assuming there are only two user processes in the system
 */

#ifndef K_PROCESS_H_
#define K_PROCESS_H_

#include "k_rtx.h"
#include "rtx.h"
#include "list.h"

/* ----- Definitions ----- */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */
#define NULL_PID 0                     /* PID of null process */

#define NUM_PROCS (NUM_TEST_PROCS + 1)
extern PROC_INIT g_proc_table[NUM_PROCS];
extern PCB *gp_current_process; /* always point to the current RUN process */

typedef int pid_t;

/* ----- Functions ----- */

void process_init(void);               /* initialize all procs in the system */
PCB *scheduler(void);                  /* pick the pid of the next to run process */
int k_release_process(void);           /* kernel release_process function */

extern U32 *alloc_stack(U32 size_b);   /* allocate stack for a process */
extern void __rte(void);               /* pop exception stack frame */
extern void set_test_procs(void);      /* test process initial set up */

/*set the state of the p_pcb to BLOCKED_ON_RESOURCE and enqueue it in the blocked_on_resource queue*/
int k_enqueue_blocked_on_resource_process(PCB *p_pcb);

/*dequeue the next available process in blocked_on_resource queue*/
PCB *k_dequeue_blocked_on_resource_process(void);

int k_enqueue_ready_process(PCB *p_pcb);
PCB* k_dequeue_ready_process(void);
PCB* k_peek_ready_process_front(void);

int k_set_process_priority(int process_id, int priority);
int k_get_process_priority(int process_id);

void k_check_preemption(void);

#endif /* ! K_PROCESS_H_ */
