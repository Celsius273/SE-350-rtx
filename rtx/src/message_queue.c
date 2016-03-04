#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "message_queue.h"

MSG_BUF *dequeue_message(void* pq) {
	return NULL;
}

void enqueue_message(MSG_BUF* p_msg, void* pq) {
	return;
}

bool is_queue_empty(void* pq) {
	return true;
}

MSG_BUF *peek_message(void* pq) {
	return NULL;
}
