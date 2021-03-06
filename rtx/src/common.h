/* @brief: common defines and structs for both kernel and user 
 * @file: common.h 
 * @author: Yiqing Huang
 * @date: 2016/02/24
 */

#ifndef COMMON_H_
#define COMMON_H_

#define LOW_MEM

#define K_MSG_ENV

/* Definitions */

#define BOOL unsigned char

#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif
#define RTX_ERR -1
#define RTX_OK 0
#define NUM_TEST_PROCS 9

/* Process IDs */
#define PID_NULL 0
#define PID_P1   1
#define PID_P2   2
#define PID_P3   3
#define PID_P4   4
#define PID_P5   5
#define PID_P6   6
#define PID_A    7
#define PID_B    8
#define PID_C    9
#define PID_SET_PRIO     10
#define PID_CLOCK        11
#define PID_KCD          12
#define PID_CRT          13
#define PID_TIMER_IPROC  14
#define PID_UART_IPROC   15
#define MAX_PID 15


/* Process Priority. The bigger the number is, the lower the priority is*/
#define HIGHEST 0
#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3
#define NULL_PRIO 4
#define IPROC_PRIO 5
#define NUM_PRIORITIES 6

/* Defining the Hot Keys for debug information */

#define HOTKEY_READY_QUEUE '!'
#define HOTKEY_BLOCKED_MEM_QUEUE '@'
#define HOTKEY_BLOCKED_MSG_QUEUE '#'


/* Message Types */
#define DEFAULT 0
#define KCD_REG 1
#define KCD_KEYBOARD_INPUT 2
#define CRT_DISPLAY 3
#define COUNT_REPORT 4
#define WAKEUP_10 5

#define NO_CHAR (-1)

/* ----- Types ----- */
typedef unsigned char U8;
typedef unsigned int U32;

/* common data structures in both kernel and user spaces */

/* initialization table item, exposed to user space */
typedef struct proc_init
{	
	int m_pid;	        /* process id */ 
	int m_priority;         /* initial priority, not used in this example. */ 
	int m_stack_size;       /* size of stack in words */
	void (*mpf_start_pc) ();/* entry point of the process */    
} PROC_INIT;

/* message buffer */
typedef struct msgbuf
{
#ifdef K_MSG_ENV
	void *mp_next;		/* ptr to next message received*/
	int m_send_pid;		/* sender pid */
	int m_recv_pid;		/* receiver pid */
	int m_kdata[5];		/* extra 20B kernel data place holder */
#endif
	int mtype;              /* user defined message type */
	char mtext[1];          /* body of the message */
} MSG_BUF;

#define MEM_BLOCK_SIZE 128
#define MTEXT_MAXLEN (MEM_BLOCK_SIZE - offsetof(struct msgbuf, mtext) - 1)

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

#endif // COMMON_H_
