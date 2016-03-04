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

/* ----- Definitions ----- */

#define INITIAL_xPSR 0x01000000        /* user process initial xPSR value */

#define NUM_PROCS (NUM_TEST_PROCS + 1)

typedef int pid_t;

/* ----- Functions ----- */

void process_init(void);               /* initialize all procs in the system */
int k_release_processor(void);           /* kernel release_processor function */

extern U32 *alloc_stack(U32 size_b);   /* allocate stack for a process */
extern void __rte(void);               /* pop exception stack frame */
extern void set_test_procs(void);      /* test process initial set up */

// Preempt the current process if needed.
// Does nothing if already preempted.
void k_check_preemption(void);

// System calls
int k_set_process_priority(int process_id, int priority);
int k_get_process_priority(int process_id);

/*Inter Process Communication*/
int k_send_message(int receiver_pid, void *p_msg_env);
void *k_receive_message(int *sender_id);

int k_delayed_send(int sender_pid, void *p_msg_env, int delay);

#endif /* ! K_PROCESS_H_ */
