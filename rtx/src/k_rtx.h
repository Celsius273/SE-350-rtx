/**
 * @file:   k_rtx.h
 * @brief:  kernel deinitiation and data structure header file
 * @auther: Yiqing Huang
 * @date:   2014/01/17
 */
 
#include "common.h"

#ifndef K_RTX_H_
#define K_RTX_H_

#include "allow_k.h"

/*----- Definitations -----*/

#define RTX_ERR -1
#define RTX_OK  0

#define NULL 0
#define NUM_MEM_BLOCKS 30
#define MEM_BLOCK_SIZE 128

/*----- Types -----*/
typedef unsigned char U8;
typedef unsigned int U32;

/* process states */
typedef enum {NEW = 0, RDY, RUN, BLOCKED_ON_RESOURCE, BLOCKED_ON_RECEIVE, NUM_PROC_STATES} PROC_STATE_E;

/*
  PCB data structure definition.
  You may want to add your own member variables
  in order to finish P1 and the entire project
*/
typedef struct pcb
{
	//struct pcb *mp_next;  /* next pcb, not used in this example */
	U32 *mp_sp;		/* stack pointer of the process */
	U32 m_pid;		/* process id */
	PROC_STATE_E m_state;   /* state of the process */
	int m_priority;         /* current priority */
} PCB;

#include "disallow_k.h"

#endif // ! K_RTX_H_
