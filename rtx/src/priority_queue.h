/** 
 * @file:   priority_queue.h
 * @brief:  Header definition for priority queue for sorting processes
 * @auther: Jobair Hassan, Kelvin Jiang
 * @date:   2016/01/22
 */
#ifndef PRIORITY_QUEUE_H_
#define PRIORITY_QUEUE_H_

#include <stddef.h>
#include <stdbool.h>
#include "list.h"
#include "rtx.h"
#include "k_process.h"

typedef struct pid_list{
    ll_header_t header;
    pid_t values[NUM_PROCS];
} pid_list;

typedef pid_list *pid_pq;

//#define NUM_PROCESSES 4

void push_process(void* pq, pid_t pid, int priority);

pid_t pop_process(void* pq, int priority);

pid_t pop_first_process(void* pq);

pid_t peek_process_front(void* pq, int priority); 

pid_t peek_front(void* pq, int *priority);

pid_t peek_process_back(void* pq, int priority);

bool change_priority(void* pq, pid_t pid, int from, int to);

void move_process(void* from_queue, void* to_queue, pid_t pid);

void clear_queue(void* q);

void copy_queue(void* fq, void* tq);

void print_priority_queue(void* priority_queue);

/* Your implementation of queue is highly tailored to just processes,
	 we need to be able to use it for inter process communication as well.
	 Can you please have a look at it and see how will you modify it. Thanks -Pushpak.
	 
	 Note, the following functions give you the exact list, you don't need to access particular list based on the priority 
*/

// MSG_BUF *dequeue_message(void* pq); 
// void enqueue_message(MSG_BUF* p_msg, void* pq);

#endif
