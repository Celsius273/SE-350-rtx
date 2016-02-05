#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
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

pid_t peek_front(void* pq) {
    pid_pq priority_queue = (pid_pq)pq;
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        if (LL_SIZE(priority_queue[i]) > 0) {
            return LL_FRONT(priority_queue[i]);
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

// test print function
void print_priority_queue(void* pq) {
    pid_pq priority_queue = (pid_pq)pq;
    int x;
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        printf("Priority %d:\n", i);
        LL_FOREACH(x, priority_queue[i]) {
            printf("    PID %d\n", x);
        }
    }
}

int main(void) {
    // test code

    LL_DECLARE(ptest[NUM_PRIORITIES], pid_t, NUM_PROCESSES);
    LL_DECLARE(toQueue[NUM_PRIORITIES], pid_t, NUM_PROCESSES);

    push_process(ptest, 2, 0);
    push_process(ptest, 33, 0);
    print_priority_queue(ptest);
    printf("\n");

    change_priority(ptest, 33, 0, 4);
    print_priority_queue(ptest);
    printf("\n");

    push_process(toQueue, 121, 0);
    move_process(ptest, toQueue, 33);
    print_priority_queue(ptest);
    printf("\n");

    print_priority_queue(toQueue);
}
