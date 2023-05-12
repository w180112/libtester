#include <common.h>
#include "resource_test.h"
#include "thread_test.h"

int main()
{
    puts("====================start unit tests====================\n");
    puts("====================test resource.c====================");
    test_insert_co_pid_2_list();
    puts("ok!");

    puts("====================test thread.c====================");
    test_get_all_timeout_threads_from_list();
    puts("ok!");

    puts("\nall test successfully");
    puts("====================end of unit tests====================");
}