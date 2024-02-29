#include <libtester/tester.h>

int init_test_env(thread_list_t *this_thread)
{
    FILE *log_fp = this_thread->log_info.log_fp;
    const char *branch_name = this_thread->branch_name;
    const char *script_path = this_thread->script_path;
    TEST_TYPE test_type = this_thread->test_type;

    char git_op_cmd[2*PATH_MAX] = { 0 };
    char pwd[PATH_MAX] = { 0 };
    char *init_sh_path = "/test-init.sh 2>&1";
    char *git_op_cmd_str = " && cd /root/replace-docx-field && git checkout ";

    strncpy(pwd, script_path, PATH_MAX-strlen(init_sh_path)-1);
    pwd[PATH_MAX-strlen(init_sh_path)-1] = '\0';
    strncat(pwd, init_sh_path, strlen(init_sh_path)+1);
    strncpy(git_op_cmd, pwd, strlen(pwd)+1);
    strncat(git_op_cmd, git_op_cmd_str, strlen(git_op_cmd_str)+1);
    strncat(git_op_cmd, branch_name, strlen(branch_name)+1);
    TESTER_LOG(DBG, log_fp, test_type, "init cmd = %s", git_op_cmd);
    struct thread_list *git_op = tester_new_cmd(*this_thread, git_op_cmd, "origin/master", TRUE, 90);
    tester_exec_cmd(git_op);
    if (tester_get_test_result(git_op) == ERROR) {
        tester_delete_cmd(git_op);
        TESTER_LOG(INFO, log_fp, test_type, "git operation failed");
        return ERROR;
    }
    tester_delete_cmd(git_op);
    TESTER_LOG(DBG, log_fp, test_type, "git ops done");

    return SUCCESS;
}

int start_test(thread_list_t *this_thread)
{
    int ret = 0;
    FILE *log_fp = this_thread->log_info.log_fp;
    TEST_TYPE test_type = this_thread->test_type;

    struct thread_list *run_thread = tester_new_cmd(*this_thread, "/usr/local/go/bin/go run /root/replace-docx-field/cmd/main.go -p /root/replace-docx-field/template/*", "443", TRUE, 60);
    if (run_thread == NULL) {
        TESTER_LOG(INFO, log_fp, test_type, "clean failed");
        return -1;
    }
    tester_start_cmd(run_thread);
    sleep(15);
    ret = tester_get_test_result(run_thread);
    tester_stop_cmd(run_thread);

    return ret;
}

int test_timeout(thread_list_t *this_thread)
{
    return TRUE;
}

int main(int argc, char **argv)
{
    char *test_type[] = {"go-docx-replacer"};

    struct tester_cmd tester_cmd = {
        .test_types = test_type,
        .test_type_count = 1,
        .allow_test_able_rerun = FALSE,
        .init_func = init_test_env,
        .test_func = start_test,
        .clean_func = test_timeout
    };

    int args_cnt = tester_parse_args(argc, argv);
    if (args_cnt < 0)
        return -1;
    argc += args_cnt;
    argv += args_cnt;

    if (tester_start(tester_cmd) < 0)
        return -1;
    return 0;
}