#include "list.h"
#include <assert.h>
#include <string.h>
// #include <stdio.h>

void test_my_list(void) {
	LL_DECLARE(my_list, int, 10);
	LL_DECLARE(my_list_copy, int, 10);
	LL_DECLARE(my_list_arr[3], int, 10);

	assert(LL_SIZE(my_list) == 0);
	// Push 0 to 4 to front
	for (int i = 0; i < 5; ++i) {
		// Either syntax works
		if (i % 2 == 0) {
			LL_PUSH_FRONT(my_list, i);
		} else {
			LL_PUSH_FRONT_(my_list) = i;
		}
	}
	// List is: 4 3 2 1 0
	for (int i = 5; i < 10; ++i) {
		if (i % 2 == 0) {
			LL_PUSH_BACK(my_list, i);
		} else {
			LL_PUSH_BACK_(my_list) = i;
		}
	}
	// List is: 4 3 2 1 0 5 6 7 8 9
	assert(LL_SIZE(my_list) == 10);

	assert(sizeof(my_list) == sizeof(my_list_copy));
	memcpy(&my_list_copy, &my_list, sizeof(my_list_copy));
	for (int i = 0; i < 3; ++i) {
		memcpy(&my_list_arr[i], &my_list, sizeof(my_list_arr[i]));
	}

	{
		int sum = 0;
		int x; // must declare before LL_FOREACH
		LL_FOREACH(x, my_list) {
			sum += x;
		}
		assert(sum == 45);
	}
	// Pop everything from front
	for (int i = 0; i < 10; ++i) {
		int front = LL_FRONT(my_list);
		int x = LL_POP_FRONT(my_list);
		assert(front == x);
		if (i < 5) {
			assert(x == 4 - i);
		} else {
			assert(x == i);
		}
	}

	assert(LL_SIZE(my_list) == 0);
	assert(LL_SIZE(my_list_copy) == 10);
	LL_REMOVE(my_list_copy, 5);
	assert(LL_SIZE(my_list_copy) == 9);
	LL_REMOVE(my_list_copy, 5);
	assert(LL_SIZE(my_list_copy) == 9);
	LL_REMOVE(my_list_copy, 4);
	assert(LL_SIZE(my_list_copy) == 8);
	for (int i = 0; i < 10; ++i) {
		LL_REMOVE(my_list_copy, i);
	}
	assert(LL_SIZE(my_list_copy) == 0);
	LL_REMOVE(my_list_copy, 4);
	assert(LL_SIZE(my_list_copy) == 0);
	for (int i = 0; i < 3; ++i) {
		assert(LL_SIZE(my_list_arr[i]) == 10);
	}	

	// printf("Front: %d \n", LL_FRONT(my_list));
}

int main(void) {
	test_my_list();
	test_my_list();
}
