#ifndef LINKED_LIST_H_
#define LINKED_LIST_H_

#include <stddef.h>

typedef struct ll_header {
	int head;
	int next[0];
} ll_header_t;

#define LL_DECLARE(list, type, capacity_) \
	struct { \
		ll_header_t header; \
		int next[capacity_]; \
		type values[capacity_]; \
	} list = { .header = { .head = -1 }, .next = { -1 } }

#define LL_CAPACITY(list) (sizeof((list).next) / sizeof((list).next[0]))

size_t ll_size_impl(const ll_header_t *header);
#define LL_SIZE(list) ll_size_impl(&(list).header)

int ll_push_front_impl(ll_header_t *header);
#define LL_PUSH_FRONT(list, value) ((list).values[ll_push_front_impl(&(list).header)] = (value))

int ll_push_back_impl(ll_header_t *header);
#define LL_PUSH_BACK(list, value) ((list).values[ll_push_back_impl(&(list).header)] = (value))

int ll_pop_front_impl(ll_header_t *header);
#define LL_POP_FRONT(list) ((list).values[ll_pop_front_impl(&(list).header)])

int ll_front_impl(ll_header_t *header);
#define LL_FRONT(list) ((list).values[ll_front_impl(&(list).header)])

#define LL_FOREACH(x, list) \
	for ( \
		int x ## _index = (list).header.head; \
		x ## _index != -1 && (x = (list).values[x ## _index], 1); \
		x ## _index = (list).next[x ## _index] \
	)

#endif
