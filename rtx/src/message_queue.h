/** 
 * @file:   message_queue.h
 * @brief:  Header definition for sorted list for storing messages
 * @auther: Kelvin Jiang
 * @date:   2016/03/03
 */
#ifndef MESSAGE_QUEUE_H_
#define MESSAGE_QUEUE_H_

#include <stddef.h>
#include <stdbool.h>
#include "list.h"
#include "rtx.h"
#include "k_process.h"

/*
typedef struct msgbuf
{
	void *mp_next;		// ptr to next message received
	int m_send_pid;		// sender pid
	int m_recv_pid;		// receiver pid
	int m_kdata[5];		// extra 20B kernel data place holder
	int mtype;          // user defined message type
	char mtext[1];      // body of the message
} MSG_BUF;
*/

// dequeue like a queue
MSG_BUF *dequeue_message(MSG_BUF** p_msg); 

void enqueue_message(MSG_BUF* p_msg, MSG_BUF** pq);
// check message (in front)
// is_empty (different from pid)
bool is_queue_empty(MSG_BUF const *const * p_msg);

MSG_BUF *peek_message(MSG_BUF*const * p_msg);
// looks at the message at the front of the queue

// TODO: add something that finds the first message at/past a given timestamp 


// bug
#endif
