// Unit test code:
#include <assert.h>
#include <stdlib.h>
#include "../src/thread.h"

void test_get_all_timeout_threads_from_list() {
    thread_list_t *thread_1 = malloc(sizeof(thread_list_t));
    thread_list_t *thread_2 = malloc(sizeof(thread_list_t));
    thread_list_t *thread_3 = malloc(sizeof(thread_list_t));
    thread_list_t *thread_4 = malloc(sizeof(thread_list_t));
    thread_list_t *thread_5 = malloc(sizeof(thread_list_t));
    thread_list_t *thread_6 = malloc(sizeof(thread_list_t));
    
    thread_1->test_type = 0;
    thread_1->next = thread_2;
    thread_2->test_type = 1;
    thread_2->next = thread_3;
    thread_3->test_type = 1;
    thread_3->next = thread_4;
    thread_4->test_type = 3;
    thread_4->next = thread_5;
    thread_5->test_type = 2;
    thread_5->next = thread_6;
    thread_6->test_type = 1;
    thread_6->next = NULL;

    thread_list_head = thread_1;

    thread_list_t *timeout_threads = NULL;
    get_all_timeout_threads_from_list(thread_6, &timeout_threads);

    /* test thread list contains TEST_TYPE_1 thread nodes after TEST_TYPE_1 test timeout */
    assert(timeout_threads != NULL);
    assert(thread_1 != NULL);
    assert(thread_1->next == thread_4);
    assert(thread_5->next == NULL);
    assert(timeout_threads == thread_6);
    assert(timeout_threads->next == thread_3);
    assert(timeout_threads->next->next == thread_2);
    assert(timeout_threads->next->next->next == NULL);

    thread_list_t *prev = NULL, *cur = thread_1;
    while(cur) {
        prev = cur;
        cur = cur->next;
        free(prev);
    }

    prev = NULL;
    cur = timeout_threads;
    while(cur) {
        prev = cur;
        cur = cur->next;
        free(prev);
    }
}