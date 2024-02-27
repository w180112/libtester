#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <uuid/uuid.h>
#include "tester.h"
#include "sock.h"
#include "init.h"
#include "dbg.h"
#include "exec.h"
#include "thread.h"
#include "resource.h"
#include "parse.h"

extern BOOL is_test_able_rerun;
extern char **total_test_types;
extern TEST_TYPE total_test_types_count;
struct cmd_opt options = {
    .service_logfile_path = "\0",
    .default_script_path = "\0",
    .service_config_path = "\0",
};

void timeout_handler(struct thread_list *timeout_thread)
{
    FILE *log_fp = timeout_thread->log_info.log_fp;
    TEST_TYPE test_type = timeout_thread->test_type;

    timeout_thread->clean_func(timeout_thread);

    pthread_mutex_lock(&thread_list_lock);
    if (is_thread_in_list(timeout_thread) == TRUE) {
        TESTER_LOG(INFO, timeout_thread->log_info.log_fp, test_type, "test [%s] timeout", timeout_thread->exec_cmd.cmd);
        drv_xmit((U8 *)http_fail_header, strlen(http_fail_header)+1, timeout_thread->sock);
        /* we need to get log_fp in advance because timeout_thread memory region will be free */
        remove_all_timeout_threads_from_list(timeout_thread);
        timeout_thread = NULL;
        fclose(log_fp);
    }
    pthread_mutex_unlock(&thread_list_lock);
}

void end_test_manually(uuid_t test_uuid)
{
    pthread_mutex_lock(&thread_list_lock);
    thread_list_t *thread = NULL;
    get_thread_by_uuid(test_uuid, &thread);
    remove_all_threads_in_list(thread);
    pthread_mutex_unlock(&thread_list_lock);
    thread->clean_func(thread);
}

int tester_start(struct tester_cmd tester_cmd)
{
    tTESTER_MBX	*mail;
    tMBUF       mbuf;
    int	        msize;
    U16	        ipc_type;
    tIPC_PRIM   *ipc_prim;
    STATUS      ret;
    char        **test_types = tester_cmd.test_types;
    int         test_type_count = tester_cmd.test_type_count;
    BOOL        allow_test_able_rerun = tester_cmd.allow_test_able_rerun;
    STATUS      (*init_func)(struct thread_list *this_thread) = (STATUS(*)(struct thread_list *))tester_cmd.init_func;
    STATUS      (*test_func)(struct thread_list *this_thread) = (STATUS(*)(struct thread_list *))tester_cmd.test_func;
    STATUS      (*clean_func)(struct thread_list *this_thread) = (STATUS(*)(struct thread_list *))tester_cmd.clean_func;

    if (test_types == NULL) {
        TESTER_LOG(INFO, NULL, 0, "test_types is empty");
        return -1;
    }

    FILE *log_fp;
    if (libtester_init(&q_key, options.service_logfile_path, &log_fp) == ERROR) {
        TESTER_LOG(INFO, log_fp, 0, "libtester init failed");
        return -1;
    }

    total_test_types = malloc(sizeof(char *) * test_type_count);
    if (total_test_types == NULL) {
        TESTER_LOG(INFO, log_fp, 0, "malloc total_test_types error");
        ret = -1;
        goto end;
    }
    total_test_types_count = 0;
    for (int i=0; i<test_type_count; i++) {
        total_test_types[i] = malloc((strlen(test_types[i]) + 1) * sizeof(char));
        strncpy(total_test_types[i], test_types[i], strlen(test_types[i])+1);
        TESTER_LOG(INFO, log_fp, 0, "test type: %s", total_test_types[i]);
        total_test_types_count++;
    }
    is_test_able_rerun = allow_test_able_rerun;

    pthread_t processing;
    thread_list_head = new_thread_node();
    thread_list_head->log_info.log_fp = log_fp;
    thread_list_head->test_type = 0; // ignore test type in socket thread
    thread_list_head->next = NULL;
    pthread_create(&processing, NULL, recv_req, (void *restrict)&q_key);
    thread_list_head->thread_id = processing;

    for(;;) {
        TESTER_LOG(INFO, log_fp, 0, "%s","======================== waiting for new event ========================");
        if (ipc_rcv2(q_key, &mbuf, &msize) == ERROR) {
            TESTER_LOG(INFO, log_fp, 0, "%s", "========== ipc error ==========");
            sleep(1);
            continue;
        }
        ipc_type = *(U16 *)mbuf.mtext;
        switch(ipc_type) {
        case IPC_EV_TYPE_TMR:
            ipc_prim = (tIPC_PRIM *)mbuf.mtext;
            thread_list_t *timeout_thread = (thread_list_t *)ipc_prim->ccb;
            timeout_handler(timeout_thread);
            break;
        case IPC_EV_TYPE_DRV:
            mail = (tTESTER_MBX *)mbuf.mtext;
            struct test_info test_info = *(struct test_info *)mail->refp;
            if (test_info.is_test_end == TRUE) {
                end_test_manually(test_info.test_uuid);
                break;
            }
            init_cmd(test_info, options.service_logfile_path, options.default_script_path, init_func, test_func, clean_func);
            break;
        default:
            ;
        }
    }

    pthread_join(processing, NULL);
    for(int i=0; i<total_test_types_count; i++)
        free(total_test_types[i]);
    free(total_test_types);

    ret = 0;

end:
    libtester_clean();
    fclose(log_fp);

    return ret;
}

int tester_parse_args(int argc, char **argv)
{
    tester_dbg_flag = LOGDBG;

    int args_count = parse_cmd(argc, argv, &options);
    if (args_count < 0)
        return -1;

    if (strlen(options.service_logfile_path) == 0) {
        strncpy(options.service_logfile_path, "/var/log/libtester", PATH_MAX-1);
        options.service_logfile_path[PATH_MAX-1] = '\0';
    }
    if (strlen(options.default_script_path) == 0) {
        strncpy(options.default_script_path, "/usr/local/bin/libtester", PATH_MAX-1);
        options.default_script_path[PATH_MAX-1] = '\0';
    }
    if (strlen(options.service_config_path) == 0) {
        strncpy(options.service_config_path, "/etc/libtester/config.cfg", PATH_MAX-1);
        options.service_config_path[PATH_MAX-1] = '\0';
    }
    TESTER_LOG(INFO, NULL, 0, "use %s as logfile located directory", options.service_logfile_path);
    TESTER_LOG(INFO, NULL, 0, "use %s as custom script located directory", options.default_script_path);
    TESTER_LOG(INFO, NULL, 0, "use %s as config file", options.service_config_path);

    if (parse_config(options.service_config_path) == ERROR) {
        TESTER_LOG(INFO, NULL, 0, "parse config file error");
        return -1;
    }
    TESTER_LOG(INFO, NULL, 0, "loglvl is %s", loglvl2str(tester_dbg_flag));

    return args_count;
}

int tester_get_test_info(thread_list_t *thread, struct tester_info *test_info)
{
    if (thread == NULL || test_info == NULL) {
        TESTER_LOG(INFO, NULL, 0, "input NULL pointer");
        return -1;
    }

    test_info->log_fp = thread->log_info.log_fp;
    test_info->test_type = thread->test_type;
    test_info->branch_name = thread->branch_name;
    test_info->script_path = thread->script_path;

    return 0;
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
    if (this_thread == NULL) {
        TESTER_LOG(INFO, NULL, 0, "executed thread obj is NULL");
        return;
    }
    exec_cmd(this_thread);
}

void *tester_exec_cmd_in_thread(void *arg)
{
    struct thread_list *this_thread = (struct thread_list *)arg;
    tester_exec_cmd(this_thread);

    pthread_exit(NULL);
}

void tester_start_cmd(thread_list_t *thread)
{
    if (thread == NULL) {
        TESTER_LOG(INFO, NULL, 0, "started thread obj is NULL");
        return;
    }
    add_thread_id_to_list_lock(thread);
    pthread_create(&thread->thread_id, NULL, tester_exec_cmd_in_thread, (void *restrict)thread);
}

void tester_wait_cmd_finished(thread_list_t *thread)
{
    if (thread == NULL) {
        TESTER_LOG(INFO, NULL, 0, "waiting thread obj is NULL");
        return;
    }
    pthread_join(thread->thread_id, NULL);
}

void tester_delete_cmd(thread_list_t *thread)
{
    if (thread == NULL) {
        TESTER_LOG(INFO, NULL, 0, "deleted thread obj is NULL");
        return;
    }

    kill_co_process(thread);
    if (is_thread_in_list_lock(thread) == TRUE) {
        remove_thread_id_from_list(&thread);
        return;
    }
    free(thread);
}

void tester_stop_cmd_timer(thread_list_t *thread)
{
    if (thread == NULL) {
        TESTER_LOG(INFO, NULL, 0, "stopped timer thread obj is NULL");
        return;
    }
    OSTMR_StopXtmr(thread, 0);
}

void tester_stop_cmd(thread_list_t *thread)
{
    if (thread == NULL) {
        TESTER_LOG(INFO, NULL, 0, "stopped thread obj is NULL");
        return;
    }
    pthread_cancel(thread->thread_id);
    tester_stop_cmd_timer(thread);
    tester_delete_cmd(thread);
}

int tester_get_test_result(thread_list_t *thread)
{
    if (thread == NULL) {
        TESTER_LOG(INFO, NULL, 0, "get test result thread obj is NULL");
        return ERROR;
    }
    return thread->exec_cmd.result == ERROR ? -1 : 0;
}

/**
 * @brief Create a new cmd object for execution
 * 
 * @param base_thread
 *  Executed cmd thread info
 * @param *cmd
 *  Cmd to execute
 * @param *result_check
 *  Expected output string when success
 * @param print_stdout
 *  print cmd stdout or not
 * @param timeout_sec
 *  Timeout in seconds
 * 
 * @retval The cmd objest
*/
thread_list_t *tester_new_cmd(thread_list_t base_thread, const char *cmd, const char *result_check, uint8_t print_stdout, uint16_t timeout_sec)
{
    TEST_TYPE test_type = base_thread.test_type;

    struct thread_list *new_thread = new_thread_node();
    if (new_thread == NULL) {
        TESTER_LOG(INFO, base_thread.log_info.log_fp, test_type, "allocate memory for thread object failed: %s", strerror(errno));
        return NULL;
    }
    new_thread->sock = base_thread.sock;
    new_thread->log_info.log_fp = base_thread.log_info.log_fp;
    new_thread->thread_id = 0;
    new_thread->test_type = base_thread.test_type;
    uuid_copy(new_thread->test_uuid, base_thread.test_uuid);
    new_thread->clean_func = base_thread.clean_func;
    new_thread->pid_list = NULL;
    new_thread->next = NULL;

    if (strlen(base_thread.log_info.logfile_proc_path) != 0)
        strncpy(new_thread->log_info.logfile_proc_path, base_thread.log_info.logfile_proc_path, sizeof(new_thread->log_info.logfile_proc_path)-1);
    new_thread->log_info.logfile_proc_path[sizeof(new_thread->log_info.logfile_proc_path)-1] = '\0';

    if (strlen(base_thread.script_path) != 0)
        strncpy(new_thread->script_path, base_thread.script_path, sizeof(new_thread->script_path)-1);
    new_thread->script_path[sizeof(new_thread->script_path)-1] = '\0';

    if (strlen(base_thread.branch_name) != 0)
        strncpy(new_thread->branch_name, base_thread.branch_name, sizeof(new_thread->branch_name)-1);
    new_thread->branch_name[sizeof(new_thread->branch_name)-1] = '\0';

    struct exec_cmd_info exec_cmd = {
        .print_stdout = (BOOL)print_stdout,
        .timeout_sec = (U16)timeout_sec,
    };
    if (cmd != NULL) {
        strncpy(exec_cmd.cmd, cmd, 8191);
        exec_cmd.cmd[8191] = '\0';
    }
    if (result_check != NULL) {
        strncpy(exec_cmd.result_check, result_check, 8191);
        exec_cmd.result_check[8191] = '\0';
    }
    new_thread->exec_cmd = exec_cmd;

    return new_thread;
}