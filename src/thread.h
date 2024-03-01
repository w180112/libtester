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
    BOOL print_stdout;
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

struct thread_list {
    pthread_t thread_id;
    struct log_info log_info;
    struct pid_list *pid_list;
    struct exec_cmd_info exec_cmd;
    int sock;
    tIPC_ID     q_key;
    TEST_TYPE test_type;
    uuid_t test_uuid;
    char script_path[PATH_MAX];
    char branch_name[128];
    STATUS (*clean_func) (struct thread_list *this_thread);
    struct thread_list *next;
};

extern struct thread_list *thread_list_head;
extern pthread_mutex_t thread_list_lock;
void add_thread_id_to_list(struct thread_list *new_thread);
void add_thread_id_to_list_lock(struct thread_list *new_thread);
BOOL is_thread_in_list(struct thread_list *thread);
BOOL is_thread_in_list_lock(struct thread_list *thread);
void remove_thread_id_from_list(struct thread_list **rm_thread);
BOOL is_test_running(TEST_TYPE test_type);
void get_thread_by_uuid(uuid_t test_uuid, struct thread_list **threads);
void get_all_timeout_threads_from_list(struct thread_list *timeout_thread, struct thread_list **timeout_list);
void remove_all_threads_in_list(struct thread_list *del_list);
void remove_all_timeout_threads_from_list(struct thread_list *timeout_thread);
void remove_all_timeout_threads_from_list_lock(struct thread_list *timeout_thread);
struct thread_list *new_thread_node();

#endif