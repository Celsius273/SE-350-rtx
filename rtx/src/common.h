/* @brief: common defines and structs for both kernel and user 
 * @file: common.h 
 * @author: Yiqing Huang
 * @date: 2016/02/24
 */

#ifndef COMMON_H_
#define COMMON_H_

/* Definitions */

#define BOOL unsigned char

#define TRUE 1
#define FALSE 0
#ifndef NULL
#define NULL 0
#endif
#define RTX_ERR -1
#define RTX_OK 0
#define NUM_TEST_PROCS 6

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


/* Process Priority. The bigger the number is, the lower the priority is*/
#define HIGH    0
#define MEDIUM  1
#define LOW     2
#define LOWEST  3
#define NULL_PRIO 4
#define NUM_PRIORITIES 5

/* Message Types */
#define DEFAULT 0
#define KCD_REG 1
#define CRT_DISPLAY 2

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
	void *mp_next;		/* ptr to next message received*/
	int m_send_pid;		/* sender pid */
	int m_recv_pid;		/* receiver pid */
	int m_kdata[5];		/* extra 20B kernel data place holder */
	int mtype;              /* user defined message type */
	char mtext[1];          /* body of the message */
} MSG_BUF;

#define MSG_HEADER_OFFSET 29  //this needs to be updated once timing delay is added

#ifdef DEBUG_0
#define USR_SZ_STACK 0x200         /* user proc stack size 512B   */
#else
#define USR_SZ_STACK 0x100         /* user proc stack size 218B  */
#endif /* DEBUG_0 */

#endif // COMMON_H_
