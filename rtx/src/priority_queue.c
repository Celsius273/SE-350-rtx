#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "list.h"
#include "priority_queue.h"

void push_process(pid_pq priority_queue, pid_t pid, int priority) {
    LL_PUSH_BACK(priority_queue[priority], pid);
}

pid_t pop_process(pid_pq priority_queue, int priority) {
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return NULL;
    }
    return LL_POP_FRONT(priority_queue[priority]);
}

pid_t pop_first_process(pid_pq priority_queue) {
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        if (LL_SIZE(priority_queue[i]) > 0) {
            return LL_POP_FRONT(priority_queue[i]);
        }
    }
    return NULL;
}

pid_t peek_process_front(pid_pq priority_queue, int priority) {
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return NULL;
    }
    return LL_FRONT(priority_queue[priority]);
}   

pid_t peek_front(pid_pq priority_queue) {
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        if (LL_SIZE(priority_queue[i]) > 0) {
            return LL_FRONT(priority_queue[i]);
        }
    }
    return NULL;
}  

pid_t peek_process_back(pid_pq priority_queue, int priority) {
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return NULL;
    }
    return LL_BACK(priority_queue[priority]);
}   

bool change_priority(pid_pq priority_queue, pid_t pid, int from, int to) {
    int orig_size = LL_SIZE(priority_queue[from]);

    LL_REMOVE(priority_queue[from], pid);

    if (LL_SIZE(priority_queue[from]) == orig_size) {
        return false;
    }
    LL_PUSH_BACK(priority_queue[to], pid);
    return true;
}

void move_process(pid_pq from_queue, pid_pq to_queue, pid_t pid) {
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

/*
// test print function
void print_priority_queue(pid_pq priority_queue) {
    int x;
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        printf("Priority %d:\n", i);
        LL_FOREACH(x, priority_queue[i]) {
            printf("    PID %d\n", x);
        }
    }
}
*/

/*
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
*/
