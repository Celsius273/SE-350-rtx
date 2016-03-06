#ifndef LIST_H_
#define LIST_H_

#include <stddef.h>

/*
 * The header of a generic linear list.
 * This is for internal use.
 *
 * Every list conceptually has a linear shape:
 * [front, item, item, ..., item, item, back]
 *
 * The implementation uses a deque.
 */
typedef struct ll_header {
	int start, size;
	int next[0];
} ll_header_t;

/**
 * LL_DECLARE(my_list, type (e.g. int), capacity (e.g. 30));
 * Declare a list with a fixed capacity (maximum size).
 * Lists hold an ordered sequence of values.
 * Initially, the size is 0.
 */

// As a special case, if .used_front is negative, then the list needs to be initialized.
#define LL_DECLARE(list, type, capacity_) \
	struct { \
		ll_header_t header; \
		type values[capacity_]; \
	} list = {0}

/**
 * size_t capacity = LL_CAPACITY(my_list);
 * Get the capacity of a list.
 */
#define LL_CAPACITY(list) (sizeof((list).values) / sizeof((list).values[0]))

/**
 * size_t size = LL_SIZE(list);
 * Get the current size of the list.
 */
#define LL_SIZE(list) ((list).header.size)

int ll_push_front_impl(volatile ll_header_t *header, int capacity);
/**
 * LL_PUSH_FRONT(list) = value;
 * Push value onto the front of the list.
 */
#define LL_PUSH_FRONT_(list) ((list).values[ll_push_front_impl(&(list).header, LL_CAPACITY((list)))])
/**
 * LL_PUSH_FRONT(list, value);
 * Push value onto the front of the list.
 */
#define LL_PUSH_FRONT(list, value) (LL_PUSH_FRONT_(list) = (value))

int ll_push_back_impl(volatile ll_header_t *header, int capacity);
/**
 * LL_PUSH_BACK(list) = value;
 * Push value onto the back of the list.
 */
#define LL_PUSH_BACK_(list) ((list).values[ll_push_back_impl(&(list).header, LL_CAPACITY((list)))])
/**
 * LL_PUSH_BACK(list, value);
 * Push value onto the back of the list.
 */
#define LL_PUSH_BACK(list, value) (LL_PUSH_BACK_(list) = (value))

int ll_pop_front_impl(volatile ll_header_t *header, int capacity);
/**
 * value = LL_POP_FRONT(list);
 * Pop value off the front of the list.
 */
#define LL_POP_FRONT(list) ((list).values[ll_pop_front_impl(&(list).header, LL_CAPACITY((list)))])

int ll_pop_back_impl(volatile ll_header_t *header, int capacity);
/**
 * value = LL_POP_BACK(list);
 * Pop value off the front of the list.
 */
#define LL_POP_BACK(list) ((list).values[ll_pop_back_impl(&(list).header, LL_CAPACITY((list)))])

int ll_front_impl(volatile ll_header_t *header, int capacity);
/**
 * value = LL_FRONT(list);
 * Peek at value off the front of the list, without popping it.
 */
#define LL_FRONT(list) ((list).values[ll_front_impl(&(list).header, LL_CAPACITY((list)))])

int ll_back_impl(volatile ll_header_t *header, int capacity);
/**
 * value = LL_BACK(list);
 * Peek at value off the back of the list, without popping it.
 */
#define LL_BACK(list) ((list).values[ll_back_impl(&(list).header, LL_CAPACITY((list)))])

/**
 * Internal use.
 * Return the index-th value of the list.
 * index is evaluated exactly once.
 */
#define LL_AT_(list, index) ((list).values[((list).header.start + (index)) % LL_CAPACITY((list))])

/**
 * LL_FOREACH(x, list) ...
 * Perform a for-loop over the list, assigning each value to x.
 */
#define LL_FOREACH(x, list) \
	for ( \
		int x ## _index = 0; \
		x ## _index != LL_SIZE((list)) && ((x = LL_AT_((list), x ## _index)), 1); \
		++x ## _index \
	)

/**
 * LL_REMOVE(list, value);
 * Remove all instances of value from list.
 * This takes O(n) time because it copies every element of the list back.
 */
#define LL_REMOVE(list, value) do { \
	int dst = 0; \
	for (int src = 0; src != LL_SIZE((list)); ++src) {\
		if (LL_AT_((list), src) != value) { \
			LL_AT_((list), dst++) = LL_AT_((list), src); \
		} \
	} \
	LL_SIZE((list)) = dst; \
} while (0)

#endif
