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

#include "common.h"
#include "k_rtx.h"

#include "allow_k.h"

/* ----- Definitions ----- */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

#define NUM_PROCS (MAX_PID + 1)

typedef int pid_t;

/* ----- Functions ----- */

void process_init(void);               /* initialize all procs in the system */
int k_release_processor(void);           /* kernel release_processor function */

extern U32 *alloc_stack(U32 size_b);   /* allocate stack for a process */
extern void __rte(void);               /* pop exception stack frame */
extern void set_test_procs(void);      /* test process initial set up */

// recursive IRQ disabling
void enable_irq(void);
void disable_irq(void);

// Preempt the current process if needed.
// Does nothing if already preempted.
void k_check_preemption(void);
void k_check_preemption_eager(void);
void k_send_message_helper(int sender_pid, int receiver_pid, void *p_msg);
// Suspend the process until an event is triggered.
// which is one of: RDY, BLOCKED_ON_RESOURCE, or BLOCKED_ON_RECEIVE
void k_poll(PROC_STATE_E which);
// Unblock processes receiving delayed messages.
// Move the messages to the appropriate queue.
void k_check_delayed_messages(void);
int k_internal_get_process_priority(int pid);

// System calls
int k_set_process_priority(int process_id, int priority);
int k_get_process_priority(int process_id);

/*Inter Process Communication*/
int k_send_message(int receiver_pid, void *p_msg_env);
void *k_receive_message(int *sender_id);

int k_delayed_send(int sender_pid, void *p_msg_env, int delay);


#ifdef _DEBUG_HOTKEYS
void k_print_blocked_on_receive_queue(void);
void k_print_blocked_on_memory_queue(void);
void k_print_ready_queue(void);
#endif


#include "disallow_k.h"
#endif /* ! K_PROCESS_H_ */
