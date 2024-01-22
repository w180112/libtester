// Unit test code:
#include <assert.h>
#include <stdlib.h>
#include "../src/resource.h"

void test_insert_co_pid_2_list() {
    struct pid_list *co_pid_list = NULL;

    // Test 1: Inserting a new node to an empty list
    struct pid_list *new_pid1 = malloc(sizeof(struct pid_list));
    new_pid1->pid = 1;
    new_pid1->next = NULL;

    assert(insert_co_pid_2_list(&co_pid_list, new_pid1) == SUCCESS);  // Check that the function returns SUCCESS
    assert(co_pid_list != NULL);  // Check that the list is not empty after insertion
    assert(co_pid_list->next == NULL);  // Check that the list only has one element after insertion

    // Test 2: Inserting a new node to a non-empty list in order
    struct pid_list *new_pid2 = malloc(sizeof(struct pid_list));
    new_pid2->pid = 2;
    new_pid2->next = NULL;

    assert(insert_co_pid_2_list(&co_pid_list, new_pid2) == SUCCESS);  // Check that the function returns SUCCESS
    assert(co_pid_list != NULL);  // Check that the list is not empty after insertion
    assert(co_pid_list->next != NULL);  // Check that the list has two elements after insertion
    assert(co_pid_list->next->pid == 2);  // Check that the second element has been inserted in order

    // Test 3: Inserting a duplicate node to a non-empty list in order should return ERROR.
    struct pid_list *new_pid3 = malloc(sizeof(struct pid_list));
    new_pid3->pid = 2;
    new_pid3->next = NULL;

    assert(insert_co_pid_2_list(&co_pid_list, new_pid3) == ERROR);  // Check that the function returns ERROR.
    free(new_pid3);  // Free the memory allocated for the duplicate node.

    struct pid_list *prev = NULL, *cur = co_pid_list;
    while(cur) {
        prev = cur;
        cur = cur->next;
        free(prev);
    }
}