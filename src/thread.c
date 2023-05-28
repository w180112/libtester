#include <common.h>
#include <errno.h>
#include <uuid/uuid.h>
#include "thread.h"
#include "exec.h"
#include "resource.h"
#include "dbg.h"

thread_list_t *thread_list_head;
pthread_mutex_t thread_list_lock;

extern void kill_co_process(thread_list_t *target_thread);

void add_thread_id_to_list(thread_list_t *new_thread)
{
    thread_list_t *cur;

    for(cur=thread_list_head; cur!=NULL&&cur->next!=NULL; cur=cur->next);
    cur->next = new_thread;
}

void add_thread_id_to_list_lock(thread_list_t *new_thread)
{
    pthread_mutex_lock(&thread_list_lock);
    add_thread_id_to_list(new_thread);
    pthread_mutex_unlock(&thread_list_lock);
}

BOOL is_thread_in_list(thread_list_t *thread)
{
    for(thread_list_t *cur=thread_list_head; cur!=NULL; cur=cur->next) {
        if (cur == thread)
            return TRUE;
    }

    return FALSE;
}


BOOL is_thread_in_list_lock(thread_list_t *thread)
{
    BOOL ret = FALSE;

    pthread_mutex_lock(&thread_list_lock);
    ret = is_thread_in_list(thread);
    pthread_mutex_unlock(&thread_list_lock);

    return ret;
}

void remove_thread_id_from_list(thread_list_t **rm_thread)
{   
    thread_list_t *cur;

    pthread_mutex_lock(&thread_list_lock);
    cur = thread_list_head;
    while(1) {
        if (cur == NULL || cur->next == NULL)
            break;
        if (cur->next->exec_cmd.exec_pid == (*rm_thread)->exec_cmd.exec_pid && \
            uuid_compare(cur->next->test_uuid, (*rm_thread)->test_uuid) == 0) {
            cur->next = cur->next->next;
            if ((*rm_thread)->exec_cmd.exec_pid != 0 && is_process_exist((*rm_thread)->exec_cmd.exec_pid))
                kill((*rm_thread)->exec_cmd.exec_pid, SIGKILL);
            kill_co_process(*rm_thread);
            close((*rm_thread)->exec_cmd.out_fp);
            free(*rm_thread);
            *rm_thread = NULL;
        }
        cur=cur->next;
    }
    pthread_mutex_unlock(&thread_list_lock);
}

BOOL is_test_running(TEST_TYPE test_type)
{
    BOOL is_running = FALSE;

    pthread_mutex_lock(&thread_list_lock);
    for(thread_list_t *cur=thread_list_head; cur!=NULL; cur=cur->next) {
        if (cur->test_type == test_type) {
            is_running = TRUE;
            break;
        }
    }
    pthread_mutex_unlock(&thread_list_lock);

    return is_running;
}

void get_all_timeout_threads_from_list(thread_list_t *timeout_thread, thread_list_t **timeout_list)
{
    thread_list_t *next = thread_list_head, *cur = NULL, *prev = NULL;
    uuid_t timeout_uuid;
    uuid_copy(timeout_uuid, timeout_thread->test_uuid);

    while(next!=NULL) {
        cur = next;
        next=next->next;
        if (uuid_compare(timeout_uuid, cur->test_uuid) == 0) {
            cur->next = *timeout_list;
            *timeout_list = cur;
            if (prev != NULL)
                prev->next = next;
        }
        else 
            prev = cur;
    }
}

void remove_all_threads_in_list(thread_list_t *del_list)
{
    for(thread_list_t *del_cur=del_list; del_cur!=NULL;) {
        thread_list_t *del_node = del_cur;
        del_cur = del_cur->next;
        if (del_node->exec_cmd.exec_pid != 0 && is_process_exist(del_node->exec_cmd.exec_pid))
            kill(del_node->exec_cmd.exec_pid, SIGKILL);
        kill_co_process(del_node);
        close(del_node->exec_cmd.out_fp);
        if (del_node->thread_id != 0) // for unit test check, otherwise thread_id is always meaningful
            pthread_cancel(del_node->thread_id);
        free(del_node);
    }
}

void remove_all_timeout_threads_from_list(thread_list_t *timeout_thread)
{
    thread_list_t *timeout_list = NULL;

    get_all_timeout_threads_from_list(timeout_thread, &timeout_list);
    remove_all_threads_in_list(timeout_list);
}

void remove_all_timeout_threads_from_list_lock(thread_list_t *timeout_thread)
{   
    pthread_mutex_lock(&thread_list_lock);
    remove_all_timeout_threads_from_list(timeout_thread);
    pthread_mutex_unlock(&thread_list_lock);
}
