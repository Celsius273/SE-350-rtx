#include <assert.h>
#include "linear_list.h"

#ifdef DEBUG_0
static void assert_header_valid(ll_header_t *header, int capacity) {
	assert(0 <= header->start && header->start < capacity);
	assert(0 <= header->size && header->size <= capacity);
}
#else
#define assert_header_valid(...)
#endif

int ll_push_front_impl(ll_header_t *header, int capacity) {
	assert_header_valid(header, capacity);
	assert(header->size < capacity);
	++header->size;
	return header->start = (header->start + capacity - 1) % capacity;
}

int ll_push_back_impl(ll_header_t *header, int capacity) {
	assert_header_valid(header, capacity);
	assert(header->size < capacity);
	return (header->start + header->size++) % capacity;
}

int ll_front_impl(ll_header_t *header, int capacity) {
	assert_header_valid(header, capacity);
	return header->start;
}

int ll_back_impl(ll_header_t *header, int capacity) {
	assert_header_valid(header, capacity);
	return (header->start + header->size) % capacity;
}

int ll_pop_front_impl(ll_header_t *header, int capacity) {
	assert_header_valid(header, capacity);
	assert(header->size > 0);
	const int cur = header->start;
	--header->size;
	header->start = (header->start + 1) % capacity;
	return cur;
}

int ll_pop_back_impl(ll_header_t *header, int capacity) {
	assert_header_valid(header, capacity);
	assert(header->size > 0);
	const int cur = (header->start + header->size) % capacity;
	--header->size;
	return cur;
}
