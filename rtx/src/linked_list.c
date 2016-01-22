#include <assert.h>
#include "linked_list.h"

size_t ll_size_impl(const ll_header_t *header) {
	size_t size = 0;
	for (int i = header->head; i != -1; i = header->next[i]) {
		++size;
	}
	return size;
}

int ll_push_front_impl(ll_header_t *header, int capacity) {
	assert(ll_size_impl(header) < capacity);
	...
	return header->head;
}

int ll_push_back_impl(ll_header_t *header, int capacity) {
	assert(ll_size_impl(header) < capacity);
	...
	return header->head;
}

int ll_pop_front_impl(ll_header_t *header) {
	const int prev_head = header->head;
	...
	return prev_head;
}

int ll_front_impl(ll_header_t *header) {
	assert(header->head != -1);
	return header->head;
}
