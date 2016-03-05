#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "common.h"
#include "timer.h"
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

    // TODO: Jobair: the test you see at the end of the file will probably break,
    // you may want to consult me (Kelvin)
    int current_time = g_timer_count;

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
// gcc -o message_queue message_queue.c -DMESSAGE_QUEUE_TEST -Wall -g3 -fsanitize=address && valgrind --leak-check=full --track-origins=yes ./message_queue
// or, without valgrind:
// gcc -o message_queue message_queue.c -DMESSAGE_QUEUE_TEST -Wall -g3 && ./message_queue

MSG_BUF create_message(char raw_text, int raw_time) {
    MSG_BUF new_message = {
        .mtext = {raw_text}, // let's assume this is unique
        .m_kdata = {raw_time},
        .mp_next = NULL
    };    

    return new_message;
}

test_check_message(MSG_BUF* msg, char head_text, int head_time) {
    assert(msg != NULL);

    printf("\n\nResults:\n");
    printf("expected time: %d\n", head_time);
    printf( "actual time: %d\n", msg->m_kdata[0]);

    printf("\nexpected text: %c\n", head_text);
    printf(    "actual text: %c\n", msg->mtext[0]);

    assert(head_time == msg->m_kdata[0]);
    assert(head_text == msg->mtext[0]);
}

void test_check_head(message_queue_t queue, char head_text, int head_time) {
    assert(!is_queue_empty(&queue));

    MSG_BUF* queue_head = peek_message(&queue);
    test_check_message(queue_head, head_text, head_time);
}

void test(message_queue_t queue) {
    printf("TESTING ENQUEUE");
    MSG_BUF m1 = create_message('a', 1);
    enqueue_message(&m1, &queue);
    test_check_head(queue, 'a', 1);

    MSG_BUF m2 = create_message('b', 4);
    enqueue_message(&m2, &queue);
    test_check_head(queue, 'a', 1);

    MSG_BUF m3 = create_message('c', 0);
    enqueue_message(&m3, &queue);
    test_check_head(queue, 'c', 0);    

    MSG_BUF m4 = create_message('d', 3);
    enqueue_message(&m4, &queue);
    test_check_head(queue, 'c', 0);

    MSG_BUF m5 = create_message('y', 0);
    enqueue_message(&m5, &queue);
    test_check_head(queue, 'c', 0);

    assert(!is_queue_empty(&queue));

    printf("TESTING DEQUEUE");
    MSG_BUF* d1 = dequeue_message(&queue);
    test_check_message(d1, 'c', 0);

    MSG_BUF* d2 = dequeue_message(&queue);
    test_check_message(d2, 'y', 0);

    MSG_BUF* d3 = dequeue_message(&queue);
    test_check_message(d3, 'a', 1);

    MSG_BUF* d4 = dequeue_message(&queue);
    test_check_message(d4, 'd', 3);

    MSG_BUF* d5 = dequeue_message(&queue);
    test_check_message(d5, 'b', 4);

    assert(is_queue_empty(&queue));
}

/*
void test_dequeue(message_queue_t queue) {
    MSG_BUF* m1 = dequeue_message(&queue);
    dequeue_message(&queue);
    dequeue_message(&queue);
    assert(!is_queue_empty(&queue));
    // for (int i = -1; i < m1.m_kdata[0]; i++) {
        // printf("WEF\n");
    // }
    // printf( "actual time: %d\n", m1.m_kdata[0]);
    // test_check_message(m1, 'c', 0);
    // free(m1);
}
*/

int main(void) {
    message_queue_t test_queue = NULL;
    assert(is_queue_empty(&test_queue));

    test(test_queue);
    // test_dequeue(test_queue); 

    assert(is_queue_empty(&test_queue));
}

#endif
