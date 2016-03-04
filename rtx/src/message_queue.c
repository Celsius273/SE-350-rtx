#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "message_queue.h"

MSG_BUF *dequeue_message(MSG_BUF** p_msg) {
	return NULL;
}

void enqueue_message(MSG_BUF* p_msg, MSG_BUF** pq) {
	return;
}

bool is_queue_empty(MSG_BUF const *const * p_msg) {
	return NULL == *p_msg;
}

MSG_BUF *peek_message(MSG_BUF*const * p_msg) {
	assert(!is_queue_empty(p_msg));
	return NULL;
}
