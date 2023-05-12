#ifndef _RESOURCE_H_
#define _RESOURCE_H_

#include <common.h>
#include "thread.h"

struct pid_list *find_co_pid(pid_t parent_pid);
void close_logfile(thread_list_t *target_thread);
BOOL is_process_exist(pid_t pid);
void kill_co_process(thread_list_t *target_thread);
STATUS insert_co_pid_2_list(struct pid_list **co_pid_list, struct pid_list *new_pid);

#endif