#include <common.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <uuid/uuid.h>
#include "sock.h"
#include "init.h"
#include "dbg.h"
#include "thread.h"
#include "exec.h"
#include "resource.h"
#include "utils.h"

BOOL is_test_able_rerun;

#define READ 0
#define WRITE 1
extern char **environ;
/**
 * @brief exec a cmd via fork()
 * 
 * @param command
 *  the executed cmd
 * @param *infp
 *  cmd stdin
 * @param *outfp
 *  cmd stdout and stderr
 * 
 * @retval cmd execution process pid
*/
pid_t popen2(const char *command, int *infp, int *outfp)
{
    int p_stdin[2], p_stdout[2];

    if (pipe(p_stdin) != 0 || pipe(p_stdout) != 0)
        return -1;

    /* we don't need to ignore SIGCHLD because we use pclose2() to claim child */
    // signal(SIGCHLD, SIG_IGN);
    pid_t pid = fork();

    if (pid < 0)
        return pid;
    else if (pid == 0) {
        close(p_stdin[WRITE]);
        dup2(p_stdin[READ], STDIN_FILENO);
        close(p_stdin[READ]);
        close(p_stdout[READ]);
        dup2(p_stdout[WRITE], STDOUT_FILENO);
        dup2(p_stdout[WRITE], STDERR_FILENO);
        close(p_stdout[WRITE]);
        //execl("/bin/sh", "sh", "-c", command, NULL);
        char *argp[] = {"bash", "-c", NULL, NULL};
        argp[2] = (char *)command;
        execve("/bin/bash", argp, environ);
        perror("execl");
        _exit(127);
    }

    if (infp == NULL)
        close(p_stdin[WRITE]);
    else
        *infp = p_stdin[WRITE];

    if (outfp == NULL)
        close(p_stdout[READ]);
    else
        *outfp = p_stdout[READ];

    close(p_stdin[READ]);
    close(p_stdout[WRITE]);

    return pid;
}

int pclose2(pid_t pid)
{
    int internal_stat;
    waitpid(pid, &internal_stat, 0);

    return WEXITSTATUS(internal_stat);
}

BOOL is_process_running(pid_t pid, int *status)
{
    for(int i=0; i<10; i++) { // if process is still running, wait at most 10 seconds
        pid_t ret = waitpid(pid, status, WNOHANG);

        if (ret == -1) {
            if (ECHILD == errno) {
                TESTER_LOG(INFO, NULL, 0, "waitpid() for child process %d warning %s", pid, strerror(errno));
                return FALSE;
            } 
            TESTER_LOG(INFO, NULL, 0, "waitpid() for child process %d failed %s", pid, strerror(errno));
            return TRUE;
        } 
        else if (ret == 0) {
            TESTER_LOG(INFO, NULL, 0, "child process %d is still running", pid);
        } 
        else if (ret == pid) {
            return FALSE;
        }
        TESTER_LOG(DBG, NULL, 0, "waiting 10 sec for child process %d", pid);
        sleep(10);
    }

    return TRUE;
}

/**
 * @brief execute a command
 * 
 * @param this_thread
 *  the executed command info
 * 
 * @retval ERROR or SUCCESS, it will store in this_thread->result
*/
void exec_cmd(struct thread_list *this_thread)
{
    struct exec_cmd_info *exec_cmd = &this_thread->exec_cmd;
    U16 timeout_sec = this_thread->exec_cmd.timeout_sec;
    const char *cmd = exec_cmd->cmd;
    const char *result_check = exec_cmd->result_check;
    FILE *log_fp = this_thread->log_info.log_fp;
    TEST_TYPE test_type = this_thread->test_type;
    tIPC_ID q_key = this_thread->q_key;
    char cmd_stdout[65536];
    memset(cmd_stdout, 0, 65536);
    FILE *cmd_fp = NULL;

    int out_fp;
    pid_t pid;

    this_thread->exec_cmd.result = ERROR;

    TESTER_LOG(INFO, log_fp, test_type, "test [%s] command", cmd);
    TESTER_LOG(INFO, log_fp, test_type, "expected stdout result is [%s] in %u seconds", result_check, timeout_sec);

    pid = popen2(cmd, NULL, &out_fp);
    TESTER_LOG(DBG, log_fp, test_type, "main exec pid = %d", pid);

    cmd_fp = fdopen(out_fp, "r");
    exec_cmd->cmd_fp = cmd_fp;
    exec_cmd->out_fp = out_fp;
    exec_cmd->exec_pid = pid;

    sleep(1);
    this_thread->pid_list = find_co_pid(this_thread->exec_cmd.exec_pid, test_type);
    OSTMR_StartTmr(q_key, this_thread, timeout_sec*1000000, "tester:", 0);
    TESTER_LOG(DBG, log_fp, test_type, "all co pids are found");

    U32 total_len = 0;
    BOOL is_cmp_stdout = TRUE, is_stdout_correct = FALSE, is_exit_code_0 = FALSE;
    if (strlen(result_check) <= 0) {
        is_stdout_correct = TRUE;
        is_cmp_stdout = FALSE;
    }
    
    while (1) {
        if (cmd_fp == NULL || fgets(cmd_stdout, sizeof(cmd_stdout), cmd_fp) == NULL)
            break;

        if (this_thread->exec_cmd.print_stdout == TRUE) {
            printf("res: %s", cmd_stdout);
            total_len += strlen(cmd_stdout);
            if (log_fp != NULL) {
                fwrite(cmd_stdout, sizeof(char), strlen(cmd_stdout), log_fp);
                fflush(log_fp);
            }
        }
        
        if (is_cmp_stdout == FALSE)
            continue;

        if (strstr(cmd_stdout, result_check) != NULL) {
            TESTER_LOG(DBG, log_fp, test_type, "cmd finished");
            is_stdout_correct = TRUE;
            break;
        }
    }

    int cmd_status;
    if (is_process_running(pid, &cmd_status) == FALSE) {
        is_exit_code_0 = TRUE;
        if (WIFEXITED(cmd_status) && WEXITSTATUS(cmd_status) != 0) {
            is_exit_code_0 = FALSE;
            printf("Child exited with RC = %d\n", WEXITSTATUS(cmd_status));
        }
        if (WIFSIGNALED(cmd_status)) {
            is_exit_code_0 = FALSE;
            printf("Child exited via signal %d\n",WTERMSIG(cmd_status));
        }
    }
    else {
        TESTER_LOG(INFO, log_fp, test_type, "kill cmd process %d", pid);
        kill(pid, SIGKILL);
        // is_exit_code_0 = TRUE; // we already got the correct stdout result
    }
    pclose2(pid);
    if (cmd_fp != NULL) {
        fclose(cmd_fp);
        cmd_fp = NULL;
    }
    close(out_fp);

    this_thread->exec_cmd.result = is_stdout_correct == TRUE && is_exit_code_0 == TRUE ? SUCCESS : ERROR;

    OSTMR_StopXtmr(this_thread, 0);
}

static void *get_cmd_output(void *arg)
{
    test_obj_t *test_obj = (test_obj_t *)arg;
    struct thread_list *this_thread = test_obj->base_thread;
    TEST_TYPE test_type = this_thread->test_type;
    char *res_hdr = NULL;

    if (test_obj->init_func(this_thread) == ERROR) {
        TESTER_LOG(INFO, this_thread->log_info.log_fp, test_type, "init test failed");
        res_hdr = http_fail_header;
        goto end;
    }
    if (test_obj->test_func(this_thread) == ERROR) {
        TESTER_LOG(INFO, this_thread->log_info.log_fp, test_type, "test failed");
        res_hdr = http_fail_header;
        goto end;
    }

    TESTER_LOG(INFO, this_thread->log_info.log_fp, test_type, "test succeed");
    res_hdr = http_ok_header;
end:
    this_thread->clean_func(this_thread);
    drv_xmit((U8 *)res_hdr, strlen(res_hdr)+1, this_thread->sock);
    close_logfile(this_thread);
    if (this_thread != NULL)
        remove_thread_id_from_list(&this_thread);
    pthread_exit(NULL);
}

STATUS init_cmd(struct test_info test_info, char logfile_path[], char script_path[], STATUS(* init_func)(struct thread_list *this_thread), STATUS(* test_func)(struct thread_list *this_thread), STATUS(* clean_func)(struct thread_list *this_thread), tIPC_ID q_key)
{
    char logfile_proc_path[LOG_PATH_LEN] = {'\0'};
    char *http_result_code = http_fail_header;
    test_obj_t test_obj;
    FILE *log_fp = NULL;

    TEST_TYPE test_type = check_test_type(test_info.test_type);
    if (test_type == -1) {
        TESTER_LOG(INFO, NULL, 0, "test type [%s] is not supported", test_info.test_type);
        goto err;
    }

    log_fp = create_logfile(test_type, logfile_path, logfile_proc_path);
    if (log_fp == NULL)
        goto err;

    if (is_test_able_rerun == FALSE && is_test_running(test_type) == TRUE) {
        TESTER_LOG(INFO, log_fp, test_type, "test [%s] is still running", test_info.test_type);
        http_result_code = http_503_header;
        goto err;
    }

    test_obj.init_func = init_func;
    test_obj.test_func = test_func;

    /* create an empty cmd obj */
    struct log_info log_info;
    if (strlen(logfile_proc_path) != 0)
        strncpy(log_info.logfile_proc_path, logfile_proc_path, sizeof(log_info.logfile_proc_path)-1);
    log_info.logfile_proc_path[sizeof(log_info.logfile_proc_path)-1] = '\0';
    log_info.log_fp = log_fp;
    struct thread_list *test_thread = new_thread_node();
    if (test_thread != NULL) {
        memset(test_thread, 0, sizeof(struct thread_list));
        test_thread->sock = test_info.socket;
        test_thread->test_type = test_type;
        test_thread->log_info = log_info;
        test_thread->clean_func = clean_func;
        test_thread->q_key = q_key;
        if (script_path != NULL)
            strncpy(test_thread->script_path, script_path, sizeof(test_thread->script_path)-1);
        test_thread->script_path[sizeof(test_thread->script_path)-1] = '\0';
        if (strlen(test_info.branch_name) != 0)
            strncpy(test_thread->branch_name, test_info.branch_name, sizeof(test_thread->branch_name)-1);
        test_thread->branch_name[sizeof(test_thread->branch_name)-1] = '\0';
        uuid_copy(test_thread->test_uuid, test_info.test_uuid);
        add_thread_id_to_list_lock(test_thread);
        test_obj.base_thread = test_thread;
        pthread_create(&test_thread->thread_id, NULL, get_cmd_output, (void *restrict)&test_obj);
    }
    else {
        TESTER_LOG(INFO, log_fp, test_type, "generate test cmd object failed");
        goto err;
    }

    return SUCCESS;

err:
    drv_xmit((U8 *)http_result_code, strlen(http_result_code)+1, test_info.socket);
    if (log_fp != NULL) {
        fclose(log_fp);
        log_fp = NULL;
    }
    
    return ERROR;
}
