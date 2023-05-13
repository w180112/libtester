#ifndef _TESTER_H_
#define _TESTER_H_

#include <common.h>
#include "thread.h"

STATUS tester_start(int argc, char **argv, char *test_types[], int test_type_count, STATUS(* init_func)(thread_list_t *this_thread), STATUS(* test_func)(thread_list_t *this_thread));
void tester_start_cmd(struct thread_list *thread);
void tester_delete_cmd(struct thread_list *thread);
void tester_wait_cmd_finished(struct thread_list *thread);
void tester_stop_cmd_timer(struct thread_list *thread);
void tester_stop_cmd(struct thread_list *thread);
STATUS tester_get_test_result(struct thread_list *thread);
struct thread_list *tester_new_cmd(struct thread_list base_thread, const char *cmd, const char *result_check, U16 timeout_sec);

#endif