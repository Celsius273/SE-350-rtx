#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "list.h"
#include "priority_queue.h"


LL_DECLARE(priority_queue[NUM_PRIORITIES], pid_t, NUM_PROCESSES);

void push_process(pid_t pid, int priority) {
    LL_PUSH_BACK(priority_queue[priority], pid);
}

pid_t pop_process(int priority) {
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return NULL;
    }
    return LL_POP_FRONT(priority_queue[priority]);
}

pid_t pop_first_process() {
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        if (LL_SIZE(priority_queue[i]) > 0) {
            return LL_POP_FRONT(priority_queue[i]);
        }
    }
    return NULL;
}

pid_t peek_process_front(int priority) {
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return NULL;
    }
    return LL_FRONT(priority_queue[priority]);
}   

pid_t peek_front() {
    for (int i = 0; i < NUM_PRIORITIES; i++) {
        if (LL_SIZE(priority_queue[i]) > 0) {
            return LL_FRONT(priority_queue[i]);
        }
    }
    return NULL;
}  

pid_t peek_process_back(int priority) {
    if (LL_SIZE(priority_queue[priority]) == 0) {
        return NULL;
    }
    return LL_BACK(priority_queue[priority]);
}   

bool move_process(pid_t pid, int from, int to) {
    int orig_size = LL_SIZE(priority_queue[from]);

    LL_REMOVE(priority_queue[from], pid);

    if (LL_SIZE(priority_queue[from]) == orig_size) {
        return false;
    }
    LL_PUSH_BACK(priority_queue[to], pid);
    return true;
}

void print_priority_queue() {
    int x;
    for (int i = 0; i <= NUM_PRIORITIES; i++) {
        printf("Priority %d:\n", i);
        LL_FOREACH(x, priority_queue[i]) {
            printf("    PID %d\n", x);
        }
    }
}
/*
int main(void) {
    push_process(1, 0);
    print_priority_queue();



}
*/