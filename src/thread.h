#ifndef _THREAD_H_
#define _THREAD_H_

#include <common.h>
#include <uuid/uuid.h>

typedef int TEST_TYPE;

struct exec_cmd_info{
    char cmd[8192];
    char result_check[8192];
    pid_t exec_pid;
    pid_t shell_pid;
    FILE *cmd_fp;
    int out_fp;
    U16 timeout_sec;
    STATUS result;
};

struct pid_list {
    pid_t pid;
    struct pid_list *next;
};

#define PROC_PATH_LEN 32
#define LOG_PATH_LEN PROC_PATH_LEN 

struct log_info {
    char logfile_proc_path[LOG_PATH_LEN];
    FILE *log_fp;
};

typedef struct thread_list {
    pthread_t thread_id;
    struct log_info log_info;
    struct pid_list *pid_list;
    struct exec_cmd_info exec_cmd;
    int sock;
    TEST_TYPE test_type;
    uuid_t test_uuid;
    char script_path[PATH_MAX];
    char branch_name[128];
    struct thread_list *next;
}thread_list_t;

/*typedef struct test_obj {
    thread_list_t thread_info;
    void (*start_cmd) ();
    void (*wait_cmd_finished) ();
    void (*delete_cmd) ();
    void (*stop_cmd_timer) ();
    void (*stop_cmd) ();
    STATUS (*get_test_result) ();
    struct test_obj *next;
}test_obj_t;*/

extern thread_list_t *thread_list_head;
extern pthread_mutex_t thread_list_lock;
void add_thread_id_to_list(thread_list_t *new_thread);
void add_thread_id_to_list_lock(thread_list_t *new_thread);
BOOL is_thread_in_list(thread_list_t *thread);
BOOL is_thread_in_list_lock(thread_list_t *thread);
void remove_thread_id_from_list(thread_list_t **rm_thread);
BOOL is_test_running(TEST_TYPE test_type);
void get_all_timeout_threads_from_list(thread_list_t *timeout_thread, thread_list_t **timeout_list);
void remove_all_threads_in_list(thread_list_t *del_list);
void remove_all_timeout_threads_from_list(thread_list_t *timeout_thread);
void remove_all_timeout_threads_from_list_lock(thread_list_t *timeout_thread);

#endif