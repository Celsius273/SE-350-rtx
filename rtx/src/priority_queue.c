#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "printf.h"
#include "list.h"
#include "priority_queue.h"

void push_process(void* pq, pid_t pid, int priority) {
    pid_pq priority_queue = (pid_pq)pq;
    LL_PUSH_BACK(priority_queue[priority], pid);
}

pid_t pop_process(void* pq, int priority) {
    pid_pq priority_queue = (pid_pq)pq;
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return -1;
    }
    return LL_POP_FRONT(priority_queue[priority]);
}

pid_t pop_first_process(void* pq) {
    pid_pq priority_queue = (pid_pq)pq;
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        if (LL_SIZE(priority_queue[i]) > 0) {
            return LL_POP_FRONT(priority_queue[i]);
        }
    }
    return -1;
}

pid_t peek_process_front(void* pq, int priority) {
    pid_pq priority_queue = (pid_pq)pq;
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return -1;
    }
    return LL_FRONT(priority_queue[priority]);
}   

pid_t peek_front(void* pq, int *prio) {
		*prio = NUM_PRIORITIES;
    pid_pq priority_queue = (pid_pq)pq;
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        if (LL_SIZE(priority_queue[i]) > 0) {
            int pid = LL_FRONT(priority_queue[i]);
					  *prio = k_internal_get_process_priority(pid);
					  return pid;
        }
    }
    return -1;
}  

pid_t peek_process_back(void* pq, int priority) {
    pid_pq priority_queue = (pid_pq)pq;
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return -1;
    }
    return LL_BACK(priority_queue[priority]);
}   

bool change_priority(void* pq, pid_t pid, int from, int to) {
    pid_pq priority_queue = (pid_pq)pq;
    int orig_size = LL_SIZE(priority_queue[from]);

    LL_REMOVE(priority_queue[from], pid);

    if (LL_SIZE(priority_queue[from]) == orig_size) {
        return false;
    }
    LL_PUSH_BACK(priority_queue[to], pid);
    return true;
}

void move_process(void* fq, void* tq, pid_t pid) {
    pid_pq from_queue = (pid_pq)fq;
    pid_pq to_queue = (pid_pq)tq;
    int orig_size;
    int priority = -1;

    for (int i = 0; i < NUM_PRIORITIES; i++) {
        orig_size = LL_SIZE(from_queue[i]);
        LL_REMOVE(from_queue[i], pid);
        if (LL_SIZE(from_queue[i]) != orig_size) {
            priority = i;
            break;
        }
    }

    if (priority >= 0) {
        LL_PUSH_BACK(to_queue[priority], pid);
    }
}

void clear_queue(void* q) {
    pid_pq queue = (pid_pq)q;
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        LL_SIZE(queue[i]) = 0;
    }
}

void copy_queue(void* fq, void* tq) {
    pid_pq from_queue = (pid_pq)fq;
    pid_pq to_queue = (pid_pq)tq;

    // put everything in to_queue, and clear from_queue
    pid_t elt;
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        LL_FOREACH(elt, from_queue[i]) {
            LL_PUSH_BACK(to_queue[i], elt);
        }
    }

    clear_queue(fq);
}


bool is_pid_queue_empty(void* pq) {
    pid_pq queue = (pid_pq)pq;
    return LL_SIZE(*queue) == 0;
}


pid_t remove_from_queue(void* pq, pid_t pid) {
    pid_pq queue = (pid_pq)pq;
    LL_REMOVE(*queue, pid);
    return pid;
}	//unlike pop this could be anywhere in the queue


bool queue_contains_node(void* pq, pid_t pcb_id) {
    pid_pq queue = (pid_pq)pq;
    for (int i = 0; i < LL_SIZE(*queue); i++) {
        if (LL_AT_(*queue, i) == pcb_id) {
            return true;
        }
    }
    return false;
}


// test print function
void print_priority_queue(void* pq) {
    pid_pq priority_queue = (pid_pq)pq;
    int x;
    for (int i = 0; i < NULL_PRIO; i++) {
        printf("  Priority %d:", i);
        LL_FOREACH(x, priority_queue[i]) {
            printf(" %d", x);
        }
        printf("\n");
    }
}

/*
int main(void) {
    // test code

    LL_DECLARE(ptest[NUM_PRIORITIES], pid_t, NUM_PROCESSES);
    LL_DECLARE(toQueue[NUM_PRIORITIES], pid_t, NUM_PROCESSES);

    push_process(ptest, 2, 0);
    push_process(ptest, 33, 0);

    printf("PTEST\n");
    print_priority_queue(ptest);
    printf("\n");

    push_process(toQueue, 121, 0);
    move_process(ptest, toQueue, 33);

    printf("PTEST\n");
    print_priority_queue(ptest);
    printf("\n");
    
    printf("toq\n");
    print_priority_queue(toQueue);
    printf("\n");

    push_process(ptest, 3, 1);
    push_process(ptest, 4, 2);
    push_process(ptest, 5, 3);
    push_process(ptest, 53, 3);

    printf("PTEST\n");
    print_priority_queue(ptest);
    printf("\n");

    copy_queue(ptest, toQueue);

    printf("PTEST\n");
    print_priority_queue(ptest);
    printf("\n");

    printf("toq\n");
    print_priority_queue(toQueue);
}
*/
