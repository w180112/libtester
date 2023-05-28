#include <common.h>
#include <ctype.h>
#include <dirent.h> 
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

BOOL is_test_able_rerun;
char **total_test_types;
TEST_TYPE total_test_types_count;

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

    signal(SIGCHLD, SIG_IGN);
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

char *trim_string(char *str)
{
    char *end;

    while(isspace((unsigned char)*str)) 
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) 
        end--;

    end[1] = '\0';

    return str;
}

TEST_TYPE check_test_type(char *test_type)
{
    for(int i=0; i<total_test_types_count; i++) {
        if (strncmp(total_test_types[i], test_type, strlen(test_type)) == 0) {
            TESTER_LOG(INFO, NULL, i+1, "test %s", test_type);
            return i+1;
        }
    }
    
    return -1;
}

/**
 * @brief execute a command
 * 
 * @param this_thread
 *  the executed command info
 * 
 * @retval ERROR or SUCCESS, it will store in this_thread->result
*/
void tester_exec_cmd(thread_list_t *this_thread)
{
    struct exec_cmd_info *exec_cmd = &this_thread->exec_cmd;
    U16 timeout_sec = this_thread->exec_cmd.timeout_sec;
    const char *cmd = exec_cmd->cmd;
    const char *result_check = exec_cmd->result_check;
    FILE *log_fp = this_thread->log_info.log_fp;
    TEST_TYPE test_type = this_thread->test_type;
    char cmd_stdout[65536];
    memset(cmd_stdout, 0, 65536);
    FILE *cmd_fp = NULL;

    int out_fp;
    pid_t pid;

    this_thread->exec_cmd.result = ERROR;

    TESTER_LOG(INFO, log_fp, test_type, "test [%s] command", cmd);
    TESTER_LOG(INFO, log_fp, test_type, "expected result is [%s] in %u seconds", result_check, timeout_sec);

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
    
    while (1) {
        if (cmd_fp == NULL || fgets(cmd_stdout, sizeof(cmd_stdout), cmd_fp) == NULL) {
            break;
        }
        if (strlen(result_check) <= 0) {
            continue;
        }
        printf("res: %s", cmd_stdout);
        total_len += strlen(cmd_stdout);
        if (log_fp != NULL) {
            fwrite(cmd_stdout, sizeof(char), strlen(cmd_stdout), log_fp);
            fflush(log_fp);
        }
        if (strstr(cmd_stdout, result_check) != NULL) {
            TESTER_LOG(DBG, log_fp, test_type, "cmd finished");
            this_thread->exec_cmd.result = SUCCESS;
            break;
        }
    }
    TESTER_LOG(DBG, log_fp, test_type, "fwrite %u bytes to log file", total_len);
    if (cmd_fp != NULL) {
        fclose(cmd_fp);
        cmd_fp = NULL;
    }
    close(out_fp);
    kill(pid, SIGKILL);

    OSTMR_StopXtmr(this_thread, 0);
}

void *tester_exec_cmd_in_thread(void *arg)
{
    thread_list_t *this_thread = (thread_list_t *)arg;
    tester_exec_cmd(this_thread);

    pthread_exit(NULL);
}

void *get_cmd_output(void *arg)
{
    test_obj_t *test_obj = (test_obj_t *)arg;
    thread_list_t *this_thread = test_obj->base_thread;
    TEST_TYPE test_type = this_thread->test_type;

    if (test_obj->init_func(this_thread) == ERROR) {
        TESTER_LOG(INFO, this_thread->log_info.log_fp, test_type, "init test failed");
        drv_xmit((U8 *)http_fail_header, strlen(http_fail_header)+1, this_thread->sock);
        goto end;
    }
    if (test_obj->test_func(this_thread) == ERROR) {
        TESTER_LOG(INFO, this_thread->log_info.log_fp, test_type, "test failed");
        drv_xmit((U8 *)http_fail_header, strlen(http_fail_header)+1, this_thread->sock);
        goto end;
    }

    TESTER_LOG(INFO, this_thread->log_info.log_fp, test_type, "test succeed");
    drv_xmit((U8 *)http_ok_header, strlen(http_ok_header)+1, this_thread->sock);
end:
    close_logfile(this_thread);
    if (this_thread != NULL)
        remove_thread_id_from_list(&this_thread);
    pthread_exit(NULL);
}

FILE *create_logfile(TEST_TYPE test_type, char cwd[], char logfile_proc_path[])
{
    char *test_type_str = total_test_types[test_type-1];
    struct timeval tv;
    char pwd[PATH_MAX];
    memset(pwd, 0, PATH_MAX);
    strncpy(pwd, cwd, PATH_MAX-1);
    pwd[PATH_MAX-1] = '\0';
 
    gettimeofday(&tv, NULL);
    time_t t = tv.tv_sec;
    struct tm *time_format = localtime(&t);

    char logfile_name[192];
    sprintf(logfile_name, "/%s_%d-%d-%d_%d-%d-%d_%ld.log", test_type_str, time_format->tm_year+1900, 
        time_format->tm_mon+1, time_format->tm_mday, time_format->tm_hour, time_format->tm_min, time_format->tm_sec, tv.tv_usec);
    strncat(pwd, logfile_name, strlen(logfile_name)+1);
    FILE *log_fp = fopen(pwd, "w");
    if (log_fp == NULL) {
        TESTER_LOG(INFO, log_fp, test_type, "create logfile [%s] failed: %s", pwd, strerror(errno));
        return NULL;
    }
    TESTER_LOG(DBG, log_fp, test_type, "create log file [%s] for test", pwd);

    pid_t tester_pid = getpid();
    char proc_path[PROC_PATH_LEN];
    memset(proc_path, 0, PROC_PATH_LEN);
    snprintf(proc_path, PROC_PATH_LEN-1, "/proc/%d/fd/", tester_pid);
    proc_path[PROC_PATH_LEN-1] = '\0';
    DIR *d = opendir(proc_path);
    if (d) {
        struct dirent *dir;
        logfile_proc_path[0] = '\0';
        while ((dir = readdir(d)) != NULL) {
            char symlink_real_path[PATH_MAX];
            if (dir->d_type != DT_LNK)
                continue;
            memset(proc_path, 0, PROC_PATH_LEN);
            if (snprintf(proc_path, PROC_PATH_LEN-1, "/proc/%d/fd/%s", tester_pid, dir->d_name) < 0 ) {
                TESTER_LOG(INFO, log_fp, test_type, "get proc fd path %s with snprintf failed", proc_path);
                continue;
            }
            proc_path[PROC_PATH_LEN-1] = '\0';
            int path_size = readlink(proc_path, symlink_real_path, PATH_MAX-1);
            if (path_size == -1) {
                TESTER_LOG(INFO, log_fp, test_type, "read proc fd symlink %s failed", proc_path);
                continue;
            }
            symlink_real_path[path_size] = '\0';
            if (strstr(symlink_real_path, test_type_str) != NULL) {
                strncpy(logfile_proc_path, proc_path, LOG_PATH_LEN-1);
                logfile_proc_path[LOG_PATH_LEN-1] = '\0';
                break;
            }
        }
        closedir(d);
        if (strlen(logfile_proc_path) == 0) {
            TESTER_LOG(INFO, log_fp, test_type, "can't find %s file symlink in /proc/%d/fd/", logfile_name, tester_pid);
            return NULL;
        }
    }
    else {
        TESTER_LOG(INFO, log_fp, test_type, "opendir() failed: %s", strerror(errno));
        return NULL;
    }

    return log_fp;
}

STATUS init_cmd(struct test_info test_info, char logfile_path[], char script_path[], STATUS(* init_func)(thread_list_t *this_thread), STATUS(* test_func)(thread_list_t *this_thread))
{
    char logfile_proc_path[LOG_PATH_LEN];
    char *http_result_code = http_fail_header;
    test_obj_t test_obj;

    TEST_TYPE test_type = check_test_type(test_info.test_type);
    if (test_type == -1)
        goto err;

    FILE *log_fp = create_logfile(test_type, logfile_path, logfile_proc_path);
    if (log_fp == NULL)
        goto err;
    
    if (is_test_able_rerun == FALSE) {
        if (is_test_running(test_type) == TRUE) {
            TESTER_LOG(INFO, log_fp, test_type, "test [%s] is still running", test_info.test_type);
            http_result_code = http_503_header;
            goto err;
        }
    }

    test_obj.init_func = init_func;
    test_obj.test_func = test_func;
    test_obj.timeout_func = NULL;

    /* create an empty cmd obj */
    struct log_info log_info;
    if (logfile_proc_path != NULL)
        strncpy(log_info.logfile_proc_path, logfile_proc_path, sizeof(log_info.logfile_proc_path)-1);
    log_info.logfile_proc_path[sizeof(log_info.logfile_proc_path)-1] = '\0';
    log_info.log_fp = log_fp;
    struct thread_list *test_thread = (struct thread_list *)malloc(sizeof(struct thread_list));
    if (test_thread != NULL) {
        memset(test_thread, 0, sizeof(struct thread_list));
        test_thread->sock = test_info.socket;
        test_thread->test_type = test_type;
        test_thread->log_info = log_info;
        if (script_path != NULL)
            strncpy(test_thread->script_path, script_path, sizeof(test_thread->script_path)-1);
        test_thread->script_path[sizeof(test_thread->script_path)-1] = '\0';
        if (test_info.branch_name != NULL)
            strncpy(test_thread->branch_name, test_info.branch_name, sizeof(test_thread->branch_name)-1);
        test_thread->branch_name[sizeof(test_thread->branch_name)-1] = '\0';
        uuid_generate(test_thread->test_uuid);
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
