#include <common.h>
#include "resource_test.h"
#include "thread_test.h"

int main()
{
    int timer_pid = tmrInit();
    if (timer_pid == -1) {
        perror("timer init failed");
        return -1;
    }
    signal(SIGCHLD, SIG_IGN);

    puts("====================start unit tests====================\n");
    puts("====================test resource.c====================");
    test_insert_co_pid_2_list();
    puts("ok!");

    puts("====================test thread.c====================");
    test_get_all_timeout_threads_from_list();
    test_remove_thread_id_from_list();
    test_remove_all_threads_in_list();
    puts("ok!");

    puts("\nall test successfully");
    puts("====================end of unit tests====================");

    kill(timer_pid, SIGKILL);

    return 0;
}