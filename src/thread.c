#include <common.h>
#include <errno.h>
#include <uuid/uuid.h>
#include "thread.h"
#include "exec.h"
#include "resource.h"
#include "dbg.h"

struct thread_list *thread_list_head;
pthread_mutex_t thread_list_lock;

extern void kill_co_process(struct thread_list *target_thread);

void add_thread_id_to_list(struct thread_list *new_thread)
{
    struct thread_list *cur;

    for(cur=thread_list_head; cur!=NULL&&cur->next!=NULL; cur=cur->next);
    cur->next = new_thread;
}

void add_thread_id_to_list_lock(struct thread_list *new_thread)
{
    pthread_mutex_lock(&thread_list_lock);
    add_thread_id_to_list(new_thread);
    pthread_mutex_unlock(&thread_list_lock);
}

BOOL is_thread_in_list(struct thread_list *thread)
{
    for(struct thread_list *cur=thread_list_head; cur!=NULL; cur=cur->next) {
        if (cur == thread)
            return TRUE;
    }

    return FALSE;
}

BOOL is_thread_in_list_lock(struct thread_list *thread)
{
    BOOL ret = FALSE;

    pthread_mutex_lock(&thread_list_lock);
    ret = is_thread_in_list(thread);
    pthread_mutex_unlock(&thread_list_lock);

    return ret;
}

void remove_thread_id_from_list(struct thread_list **rm_thread)
{   
    struct thread_list *cur;

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
            break;
        }
        cur=cur->next;
    }
    pthread_mutex_unlock(&thread_list_lock);
}

BOOL is_test_running(TEST_TYPE test_type)
{
    BOOL is_running = FALSE;

    pthread_mutex_lock(&thread_list_lock);
    for(struct thread_list *cur=thread_list_head; cur!=NULL; cur=cur->next) {
        if (cur->test_type == test_type) {
            is_running = TRUE;
            break;
        }
    }
    pthread_mutex_unlock(&thread_list_lock);

    return is_running;
}

void get_thread_by_uuid(uuid_t test_uuid, struct thread_list **threads)
{
    struct thread_list *next = thread_list_head, *cur = NULL, *prev = NULL;

    while(next!=NULL) {
        cur = next;
        next=next->next;
        if (uuid_compare(test_uuid, cur->test_uuid) == 0) {
            cur->next = *threads;
            *threads = cur;
            if (prev != NULL)
                prev->next = next;
        }
        else 
            prev = cur;
    }
}

void get_all_timeout_threads_from_list(struct thread_list *timeout_thread, struct thread_list **timeout_list)
{
    uuid_t timeout_uuid;
    uuid_copy(timeout_uuid, timeout_thread->test_uuid);

    get_thread_by_uuid(timeout_uuid, timeout_list);
}

void remove_all_threads_in_list(struct thread_list *del_list)
{
    for(struct thread_list *del_cur=del_list; del_cur!=NULL;) {
        struct thread_list *del_node = del_cur;
        del_cur = del_cur->next;
        if (del_node->exec_cmd.exec_pid != 0 && is_process_exist(del_node->exec_cmd.exec_pid))
            kill(del_node->exec_cmd.exec_pid, SIGKILL);
        kill_co_process(del_node);
        close(del_node->exec_cmd.out_fp);
        if (del_node->thread_id != 0) // for unit test check, otherwise thread_id is always meaningful
            pthread_cancel(del_node->thread_id);
        OSTMR_StopXtmr(del_node, 0);
        free(del_node);
    }
}

void remove_all_timeout_threads_from_list(struct thread_list *timeout_thread)
{
    struct thread_list *timeout_list = NULL;

    get_all_timeout_threads_from_list(timeout_thread, &timeout_list);
    remove_all_threads_in_list(timeout_list);
}

void remove_all_timeout_threads_from_list_lock(struct thread_list *timeout_thread)
{   
    pthread_mutex_lock(&thread_list_lock);
    remove_all_timeout_threads_from_list(timeout_thread);
    pthread_mutex_unlock(&thread_list_lock);
}

struct thread_list *new_thread_node()
{
    struct thread_list *new_thread = (struct thread_list *)malloc(sizeof(struct thread_list));
    if (new_thread == NULL)
        return NULL;
    memset(new_thread, 0, sizeof(struct thread_list));

    return new_thread;
}