/** 
 * @file:   priority_queue.h
 * @brief:  Header definition for priority queue for sorting processes
 * @auther: Jobair Hassan, Kelvin Jiang
 * @date:   2016/01/22
 */
#ifndef PRIORITY_QUEUE_H_
#define PRIORITY_QUEUE_H_

#include <stddef.h>
#include "list.h"

#endif

#define NUM_PROCESSES 30
#define NUM_PRIORITIES 5

typedef int pid_t;

typedef struct pid_list{
    ll_header_t header;
    pid_t values[NUM_PROCESSES];
} pid_list;

typedef pid_list *pid_pq;

void push_process(pid_pq pq, pid_t pid, int priority);

pid_t pop_process(pid_pq pq, int priority);

pid_t pop_first_process(pid_pq pq);

pid_t peek_process_front(pid_pq pq, int priority); 

pid_t peek_front(pid_pq pq);

pid_t peek_process_back(pid_pq pq, int priority);

bool change_priority(pid_pq pq, pid_t pid, int from, int to);

void move_process(pid_pq from_queue, pid_pq to_queue, pid_t pid);

/*
void print_priority_queue(pid_pq priority_queue);
*/
