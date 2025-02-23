#ifndef _UTILS_H_
#define _UTILS_H_

#include <common.h>
#include "dbg.h"
#include "thread.h"
#include "tester.h"

static char **total_test_types;
static TEST_TYPE total_test_types_count;

static inline TEST_TYPE check_test_type(char *test_type)
{
    for(int i=0; i<total_test_types_count; i++) {
        if (strncmp(total_test_types[i], test_type, strlen(test_type)) == 0) {
            TESTER_LOG(INFO, NULL, i+1, "test %s", test_type);
            return i+1;
        }
    }
    
    return -1;
}

char *trim_string(char *str);
FILE *create_logfile(TEST_TYPE test_type, char cwd[], char logfile_proc_path[]);
void close_logfile(struct thread_list *target_thread);

#endif