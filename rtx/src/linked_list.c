#include <assert.h>
#include "linked_list.h"

static void ll_init_impl(ll_header_t *header) {
	if (header->used_front == -1 && header->used_back != -1) {
		const int capacity = header->used_back;
		assert(capacity >= 0);

		header->used_front = header->used_back = -1;
		for (int i = 0; i < capacity - 1; ++i) {
			header->next[i] = i + 1;
		}
		if (capacity > 0) {
			header->next[capacity - 1] = -1;
			header->free_front = 0;
		} else {
			header->free_front = -1;
		}
	}
}

size_t ll_size_impl(const ll_header_t *header) {
	size_t size = 0;
	for (int i = header->used_front; i != -1; i = header->next[i]) {
		++size;
	}
	return size;
}

static int ll_alloc(ll_header_t *header) {
	ll_init_impl(header);
	const int cur = header->free_front;
	assert(cur != -1);
	header->free_front = header->next[cur];
	return cur;
}

int ll_push_front_impl(ll_header_t *header) {
	const int cur = ll_alloc(header);
	header->next[cur] = header->used_front;
	header->used_front = cur;
	if (header->used_back == -1) {
		header->used_back = cur;
	}
	return cur;
}

int ll_push_back_impl(ll_header_t *header) {
	const int cur = ll_alloc(header);
	if (header->used_front == -1) {
		header->used_front = cur;
	} else {
		header->next[header->used_back] = cur;
	}
	header->next[cur] = -1;
	header->used_back = cur;
	return cur;
}

int ll_front_impl(ll_header_t *header) {
	const int cur = header->used_front;
	assert(cur != -1);
	return cur;
}

int ll_pop_front_impl(ll_header_t *header) {
	const int cur = ll_front_impl(header);
	header->used_front = header->next[cur];
	if (header->used_front == -1) {
		header->used_back = -1;
	}

	header->next[cur] = header->free_front;
	header->free_front = cur;
	return cur;
}
