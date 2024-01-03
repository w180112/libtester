#ifndef _TESTER_H_
#define _TESTER_H_

#include <common.h>
#include "thread.h"
#include "dbg.h"

typedef struct thread_list thread_list_t;

struct tester_info {
    FILE *log_fp;
    const char *branch_name;
    const char *script_path;
    TEST_TYPE test_type;
};

int tester_start(int argc, char **argv, char *test_types[], int test_type_count, BOOL allow_test_able_rerun, STATUS(* init_func)(thread_list_t *this_thread), STATUS(* test_func)(thread_list_t *this_thread), STATUS(* timeout_func)(thread_list_t *this_thread));
int tester_get_test_info(struct thread_list *thread, struct tester_info *test_info);
void tester_start_cmd(struct thread_list *thread);
void tester_delete_cmd(struct thread_list *thread);
void tester_wait_cmd_finished(struct thread_list *thread);
void tester_stop_cmd_timer(struct thread_list *thread);
void tester_stop_cmd(struct thread_list *thread);
int tester_get_test_result(struct thread_list *thread);
struct thread_list *tester_new_cmd(struct thread_list base_thread, const char *cmd, const char *result_check, BOOL print_stdout, U16 timeout_sec);
void tester_exec_cmd(struct thread_list *this_thread);
void *tester_exec_cmd_in_thread(void *arg);

#endif