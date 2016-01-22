#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <stddef.h>

/*
 * The header of a linked list.
 * Each item is either in the used or free list.
 * If the used_front == -1 but used_back != -1, then capacity is used_back and the list is uninitialized.
 * Otherwise, used_back == -1 exactly when used_front == -1.
 */
typedef struct ll_header {
	int used_front, used_back, free_front;
	int next[0];
} ll_header_t;

// As a special case, if .used_front is negative, then the list needs to be initialized.
#define LL_DECLARE(list, type, capacity_) \
	struct { \
		ll_header_t header; \
		int next[capacity_]; \
		type values[capacity_]; \
	} list = { .header = { .used_front = -1, .used_back = capacity_ } }

#define LL_CAPACITY(list) (sizeof((list).next) / sizeof((list).next[0]))

size_t ll_size_impl(const ll_header_t *header);
#define LL_SIZE(list) ll_size_impl(&(list).header)

int ll_push_front_impl(ll_header_t *header);
#define LL_PUSH_FRONT(list) ((list).values[ll_push_front_impl(&(list).header)])
#define LL_PUSH_FRONT(list, value) (LL_PUSH_FRONT(list) = (value))

int ll_push_back_impl(ll_header_t *header);
#define LL_PUSH_BACK(list) ((list).values[ll_push_back_impl(&(list).header)])
#define LL_PUSH_BACK(list, value) (LL_PUSH_BACK(list) = (value))

int ll_pop_front_impl(ll_header_t *header);
#define LL_POP_FRONT(list) ((list).values[ll_pop_front_impl(&(list).header)])

int ll_front_impl(ll_header_t *header);
#define LL_FRONT(list) ((list).values[ll_front_impl(&(list).header)])

#define LL_FOREACH(x, list) \
	for ( \
		int x ## _index = (list).header.used_front; \
		x ## _index != -1 && (x = (list).values[x ## _index], 1); \
		x ## _index = (list).next[x ## _index] \
	)

#endif
