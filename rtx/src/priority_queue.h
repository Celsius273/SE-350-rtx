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

#endif

//#define NUM_PROCESSES 4

typedef int pid_t;

typedef struct pid_list{
    ll_header_t header;
    pid_t values[NUM_PROCS];
} pid_list;

typedef pid_list *pid_pq;

void push_process(void* pq, pid_t pid, int priority);

pid_t pop_process(void* pq, int priority);

pid_t pop_first_process(void* pq);

pid_t peek_process_front(void* pq, int priority); 

pid_t peek_front(void* pq);

pid_t peek_process_back(void* pq, int priority);

bool change_priority(void* pq, pid_t pid, int from, int to);

void move_process(void* from_queue, void* to_queue, pid_t pid);

void print_priority_queue(void* priority_queue);
