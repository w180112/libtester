#ifndef _TESTER_H_
#define _TESTER_H_

#include "thread.h"
#include "dbg.h"

typedef struct thread_list thread_list_t;

struct tester_info {
    FILE *log_fp;
    const char *branch_name;
    const char *script_path;
    TEST_TYPE test_type;
};

struct tester_cmd {
    char **test_types;
    int test_type_count;
    int allow_test_able_rerun;
    int (* init_func)(thread_list_t *this_thread);
    int (* test_func)(thread_list_t *this_thread);
    int (* clean_func)(thread_list_t *this_thread);
};

int tester_parse_args(int argc, char **argv);
int tester_start(struct tester_cmd tester_cmd);
int tester_get_test_info(thread_list_t *thread, struct tester_info *test_info);
void tester_start_cmd(thread_list_t *thread);
void tester_delete_cmd(thread_list_t *thread);
void tester_wait_cmd_finished(thread_list_t *thread);
void tester_stop_cmd_timer(thread_list_t *thread);
void tester_stop_cmd(thread_list_t *thread);
int tester_get_test_result(thread_list_t *thread);
thread_list_t *tester_new_cmd(thread_list_t base_thread, const char *cmd, const char *result_check, uint8_t print_stdout, uint16_t timeout_sec);
void tester_exec_cmd(thread_list_t *this_thread);
void *tester_exec_cmd_in_thread(void *arg);

#endif