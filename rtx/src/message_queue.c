#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "message_queue.h"

/*
typedef struct msgbuf
{
    void *mp_next;      // ptr to next message received
    int m_send_pid;     // sender pid
    int m_recv_pid;     // receiver pid
    int m_kdata[5];     // extra 20B kernel data place holder
    int mtype;          // user defined message type
    char mtext[1];      // body of the message
} MSG_BUF;
*/

// function to check if it compares first k_data with g_timer_count
// if g_timer_count hasn't reached return null
// dequeue first element if it's <= timer count, so if it's supposed to be sent already
MSG_BUF *dequeue_message(MSG_BUF** msg_queue) {
    if (is_queue_empty(msg_queue)) {
        return NULL;
    }

    int current_time = 0; // TODO: Jobair: please implement g_delay_count

    MSG_BUF* msg_to_dequeue = *msg_queue;
    if (msg_to_dequeue->m_kdata[0] > current_time) {
        return NULL;
    }

    *msg_queue = (MSG_BUF*) (msg_to_dequeue->mp_next);
    msg_to_dequeue->mp_next = NULL;
    return msg_to_dequeue;
}

// we need to do something with delay
void enqueue_message(MSG_BUF* msg, MSG_BUF** msg_queue) {
    // if queue is empty
    if (is_queue_empty(msg_queue)) {
        *msg_queue = msg; // point the list to the first message
        return;
    }

    MSG_BUF* msg_to_compare = *msg_queue;
    int msg_send_at = msg->m_kdata[0];

    // special case where we insert at the beginning
    // so we have to update the msg_queue pointer
    if (msg_send_at < msg_to_compare->m_kdata[0]) {
        msg->mp_next = msg_to_compare;
        *msg_queue = msg; // point the list to the first message
        return;
    }

    // loop through case
    // let msg_to_compare be a message in the queue
    // msg_to_compare's next points to msg and msg's next points to msg_to_compare->mp_next (which could be NULL) if:
    // msg_to_compare->mp_next is NULL or msg_to_compare->mp_next->m_kdata[0] > msg->m_kdata[0]
    while (
        NULL != (MSG_BUF*) (msg_to_compare->mp_next) &&
        msg_send_at >= ((MSG_BUF*) (msg_to_compare->mp_next))->m_kdata[0]
    ) {
        msg_to_compare = (MSG_BUF*) (msg_to_compare->mp_next);
    }

    msg->mp_next = msg_to_compare->mp_next;
    msg_to_compare->mp_next = msg;
}

bool is_queue_empty(MSG_BUF const *const * msg_queue) {
    return NULL == *msg_queue;
}

MSG_BUF *peek_message(MSG_BUF*const * msg_queue) {
    assert(!is_queue_empty(msg_queue)); // idk why this is here but sure
    return *msg_queue;
}

/*
//Ted's comments
//msg->m_kdata[0] = g_timer_count;
while (!(is_queue_empty(msg_queue) ||
        (*msg_queue)->m_kdata[0] > msg->m_kdata) {
    msg_queue = &((MSG_BUF *) (*msg_queue)->mp_next);
}
msg->mp_next = *msg_queue;
*msg_queue = msg;
return;
*/

#ifdef MESSAGE_QUEUE_TEST


int main(void) {
    message_queue_t q = NULL;
    assert(is_queue_empty(&q));
    MSG_BUF m1 = {
        .mtype = 0,
        .mtext = {'b'},

        .mp_next = NULL,
        .m_send_pid = 1,
        .m_recv_pid = 2,
        .m_kdata = {1}
    };

    
}


#endif
