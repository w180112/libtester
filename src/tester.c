#include <errno.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "tester.h"
#include "sock.h"
#include "init.h"
#include "dbg.h"
#include "exec.h"
#include "thread.h"
#include "resource.h"
#include "parse.h"

STATUS tester_start(int argc, char **argv, STATUS(* init_func)(thread_list_t *this_thread), STATUS(* test_func)(thread_list_t *this_thread))
{
    tTESTER_MBX		*mail;
	tMBUF   		mbuf;
	int				msize;
	U16				ipc_type;
    tIPC_PRIM		*ipc_prim;

    struct cmd_opt options = {
        .daemon = FALSE,
        .service_logfile_path = "\0",
        .default_script_path = "\0",
    };
    tester_dbg_flag = LOGDBG;
    if (parse_cmd(argc, argv, &options) == ERROR)
        return ERROR;

    if (parse_config(options.service_config_path, "/config.cfg") == ERROR) {
        TESTER_LOG(INFO, NULL, "parse config file error");
        return ERROR;
    }
    TESTER_LOG(INFO, NULL, "loglvl is %s", loglvl2str(tester_dbg_flag));

    if (strlen(options.service_logfile_path) == 0) {
        if (getcwd(options.service_logfile_path, sizeof(options.service_logfile_path)) != NULL)
            TESTER_LOG(DBG, NULL, "Current working dir: %s", options.service_logfile_path);
        else {
            TESTER_LOG(DBG, NULL, "getcwd() error: %s", strerror(errno));
            return ERROR;
        }
    }
    if (strlen(options.default_script_path) == 0) {
        strncpy(options.default_script_path, options.service_logfile_path, PATH_MAX-strlen("/src")-1);
        options.default_script_path[PATH_MAX-strlen("/src")-1] = '\0';
        strncat(options.default_script_path, "/src", PATH_MAX-1);
        options.default_script_path[PATH_MAX-1] = '\0';
    }
    if (strlen(options.service_config_path) == 0) {
        strncpy(options.service_config_path, options.service_logfile_path, PATH_MAX-1);
        options.service_config_path[PATH_MAX-1] = '\0';
    }
    TESTER_LOG(INFO, NULL, "use %s as logfile directory", options.service_logfile_path);
    TESTER_LOG(INFO, NULL, "use %s as default script exec directory", options.default_script_path);
    TESTER_LOG(INFO, NULL, "use %s as config file directory", options.service_config_path);

    char pwd[PATH_MAX];
    char *logfile_name = "/shell-tester.log";
    strncpy(pwd, options.service_logfile_path, PATH_MAX-strlen(logfile_name)-1);
    pwd[PATH_MAX-strlen(logfile_name)-1] = '\0';

    FILE *log_fp = fopen(strncat(pwd, logfile_name, strlen(logfile_name)+1), "w");
    if (log_fp == NULL) {
        TESTER_LOG(INFO, NULL, "open tester logfile failed: %s", strerror(errno));
        return ERROR;
    }

    if (options.daemon == TRUE) {
        if (daemon(1, 0))
            TESTER_LOG(INFO, NULL, "daemonlize failed");
    }

	if (shell_tester_init(&q_key, log_fp) < 0)
		return ERROR;

    pthread_t processing;
    thread_list_head = (struct thread_list *)malloc(sizeof(struct thread_list));
    thread_list_head->log_info.log_fp = log_fp;
    thread_list_head->thread_id = processing;
    thread_list_head->test_type = 0; // ignore test type in socket thread
    thread_list_head->next = NULL;
    pthread_create(&processing, NULL, recv_req, (void *restrict)&q_key);

	for(;;) {
		TESTER_LOG(INFO, log_fp, "%s","======================== waiting for new event ========================");
        if (ipc_rcv2(q_key, &mbuf, &msize) == ERROR) {
            TESTER_LOG(INFO, log_fp, "%s", "========== ipc error ==========");
            sleep(1);
			continue;
        }
	    ipc_type = *(U16 *)mbuf.mtext;
		switch(ipc_type) {
		case IPC_EV_TYPE_TMR:
            ipc_prim = (tIPC_PRIM *)mbuf.mtext;
            thread_list_t *timeout_thread = (thread_list_t *)ipc_prim->ccb;
            if (is_thread_in_list(timeout_thread) == TRUE) {
                TESTER_LOG(INFO, timeout_thread->log_info.log_fp, "test [%s] timeout", timeout_thread->exec_cmd.cmd);
                drv_xmit((U8 *)http_fail_header, strlen(http_fail_header)+1, timeout_thread->sock);
                /* we need to get log_fp in advance because timeout_thread memory region will be free */
                FILE *log_fp = timeout_thread->log_info.log_fp;
                remove_all_timeout_threads_from_list_lock(timeout_thread);
                fclose(log_fp);
            }
			break;
		case IPC_EV_TYPE_DRV:
			mail = (tTESTER_MBX *)mbuf.mtext;
            struct test_info test_info = *(struct test_info *)mail->refp;
            init_cmd(test_info, options.service_logfile_path, options.default_script_path, init_func, test_func);
			break;
		default:
		    ;
		}
    }

    pthread_join(processing, NULL);

    return SUCCESS;
}


void tester_start_cmd(struct thread_list *thread)
{
    if (thread == NULL) {
        TESTER_LOG(INFO, NULL, "started thread obj is NULL");
        return;
    }
    add_thread_id_to_list(thread);
    pthread_create(&thread->thread_id, NULL, tester_exec_cmd_in_thread, (void *restrict)thread);
}

void tester_wait_cmd_finished(struct thread_list *thread)
{
    if (thread == NULL)
        return;
    pthread_join(thread->thread_id, NULL);
}

void tester_delete_cmd(struct thread_list *thread)
{
    if (thread == NULL)
        return;
    if (is_thread_in_list(thread) == TRUE) {
        remove_thread_id_from_list(thread);
        return;
    }
    kill_co_process(thread);
    free(thread);
}

void tester_stop_cmd_timer(struct thread_list *thread)
{
    if (thread == NULL)
        return;
    OSTMR_StopXtmr(thread, 0);
}

void tester_stop_cmd(struct thread_list *thread)
{
    if (thread == NULL)
        return;
    if (is_process_exist(thread->exec_cmd.exec_pid))
        kill(thread->exec_cmd.exec_pid, SIGKILL);
    pthread_cancel(thread->thread_id);
    tester_stop_cmd_timer(thread);
    tester_delete_cmd(thread);
}

STATUS tester_get_test_result(struct thread_list *thread)
{
    if (thread == NULL)
        return ERROR;
    return thread->exec_cmd.result;
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
 * @param timeout_sec
 *  Timeout in seconds
 * 
 * @retval The cmd objest
*/
struct thread_list *tester_new_cmd(struct thread_list base_thread, const char *cmd, const char *result_check, U16 timeout_sec)
{
    struct thread_list *new_thread = (struct thread_list *)malloc(sizeof(struct thread_list));
    if (new_thread == NULL) {
        TESTER_LOG(INFO, base_thread.log_info.log_fp, "allocate memory for thread object failed: %s", strerror(errno));
        return NULL;
    }
    new_thread->sock = base_thread.sock;
    new_thread->log_info.log_fp = base_thread.log_info.log_fp;
    new_thread->thread_id = 0;
    new_thread->test_type = base_thread.test_type;
    new_thread->pid_list = NULL;
    new_thread->next = NULL;

    if (base_thread.log_info.logfile_proc_path != NULL)
        strncpy(new_thread->log_info.logfile_proc_path, base_thread.log_info.logfile_proc_path, sizeof(new_thread->log_info.logfile_proc_path)-1);
    new_thread->log_info.logfile_proc_path[sizeof(new_thread->log_info.logfile_proc_path)-1] = '\0';

    if (base_thread.script_path != NULL)
        strncpy(new_thread->script_path, base_thread.script_path, sizeof(new_thread->script_path)-1);
    new_thread->script_path[sizeof(new_thread->script_path)-1] = '\0';

    if (base_thread.branch_name != NULL)
        strncpy(new_thread->branch_name, base_thread.branch_name, sizeof(new_thread->branch_name)-1);
    new_thread->branch_name[sizeof(new_thread->branch_name)-1] = '\0';

    struct exec_cmd_info exec_cmd = {
        .timeout_sec = timeout_sec,
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