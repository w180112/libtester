#ifndef _EXEC_H_
#define _EXEC_H_

#include <common.h>
#include "sock.h"
#include "thread.h"

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

TEST_TYPE check_test_type(char *test_type);
STATUS init_cmd(struct test_info test_info, char logfile_path[], char script_path[], STATUS(* init_func)(struct thread_list *this_thread), STATUS(* test_func)(struct thread_list *this_thread), STATUS(* clean_func)(struct thread_list *this_thread), tIPC_ID q_key);
void exec_cmd(struct thread_list *this_thread);

typedef struct test_obj {
    STATUS (*init_func)(struct thread_list *this_thread);
    STATUS (*test_func)(struct thread_list *this_thread);
    struct thread_list *base_thread;
}test_obj_t;

#endif