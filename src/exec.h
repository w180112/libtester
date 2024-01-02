#ifndef _EXEC_H_
#define _EXEC_H_

#include <common.h>
#include "tester.h"
#include "sock.h"
#include "thread.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

TEST_TYPE check_test_type(char *test_type);
STATUS init_cmd(struct test_info test_info, char logfile_path[], char script_path[], STATUS(* init_func)(thread_list_t *this_thread), STATUS(* test_func)(thread_list_t *this_thread), STATUS(* timeout_func)(thread_list_t *this_thread));
void tester_exec_cmd(thread_list_t *this_thread);
void *tester_exec_cmd_in_thread(void *arg);

typedef struct test_obj {
    STATUS (*init_func)(thread_list_t *this_thread);
    STATUS (*test_func)(thread_list_t *this_thread);
    STATUS (*timeout_func) (thread_list_t *this_thread);
    thread_list_t *base_thread;
}test_obj_t;

#endif