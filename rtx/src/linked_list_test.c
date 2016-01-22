#include "linked_list.h"
#include <assert.h>

void test_my_list(void) {
	LL_DECLARE(my_list, int, 10);

    assert(LL_SIZE(my_list) == 0);
    // Push 0 to 4 to front
    for (int i = 0; i < 5; ++i) {
        LL_PUSH_FRONT(my_list, i);
    }
    // List is: 4 3 2 1 0
    for (int i = 5; i < 10; ++i) {
        LL_PUSH_BACK(my_list, i);
    }
    // List is: 4 3 2 1 0 5 6 7 8 9
    assert(LL_SIZE(my_list) == 10);
    {
        int sum = 0;
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
}

int main(void) {
	test_my_list();
	test_my_list();
}
