#include <sys/stat.h>
#include <dirent.h>
#include "thread.h"
#include "dbg.h"


#define insert_list_node(type, head, target, new_node, target_element) \
    find_indirect_node(type, head, target, target_element) \
    new_node->next = (*p)->next; \
    *p = new_node;

#define remove_list_node(type, head, target, target_element) \
    find_indirect_node(type, head, target, target_element) \
    *p = target->next;

#define append_list_node(type, head, new_node) \
    type **p = &head; \
    while (*p != NULL) \
        p = &(*p)->next; \
    *p = new_node;

#define find_indirect_node(type, head, target, target_element) \
    type **p = &head; \
    while ((*p) != NULL && (*p)->target_element != target->target_element) \
        p = &(*p)->next;

STATUS insert_co_pid_2_list(struct pid_list **co_pid_list, struct pid_list *new_pid)
{
    if (*co_pid_list == NULL) {
        *co_pid_list = new_pid;
        return SUCCESS;
    }
    struct pid_list *cur=*co_pid_list;
    struct pid_list *prev=NULL;
    /* insert pid to list by inorder */
    while(cur!=NULL && cur->pid<new_pid->pid) {
        prev = cur;
        cur = cur->next;
    }
    if (prev == NULL) {
        new_pid->next = *co_pid_list;
        *co_pid_list = new_pid;
    }
    /* we need to skip inserting same pid node */
    else if (cur != NULL && cur->pid == new_pid->pid) {
        return ERROR;
    }
    else {
        prev->next = new_pid;
        new_pid->next = cur;
    }
    return SUCCESS;
}

/**
 * @brief Find dedicated process's offspring processes
 * 
 * @param parent_pid
 *  dedicated process id
 * 
 * @retval All offspring processes in a linked list
*/
struct pid_list *find_co_pid(pid_t parent_pid, TEST_TYPE test_type)
{
    const char *proc_dir = "/proc";
    const char *pid_stat_path = "/proc/%s/stat";
    struct pid_list *co_pid_list = NULL, *parent_co_pids = NULL;

    DIR *d;
    struct dirent *dir;

    TESTER_LOG(DBG, NULL, test_type, "looking for pid %d's co pid", parent_pid);
    d = opendir(proc_dir);
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            char proc_dir_name[270];
            sprintf(proc_dir_name, pid_stat_path, dir->d_name);
            FILE *fp = fopen(proc_dir_name, "r");
            if (fp == NULL)
                continue;
            int pid;
            char comm[1000];
            char state;
            int ppid;
            int gpid;
            fscanf(fp, "%d %s %c %d %d", &pid, comm, &state, &ppid, &gpid);
            fclose(fp);
            if (ppid == parent_pid) {
                TESTER_LOG(DBG, NULL, test_type, "find co pids = %d, pid name is: %s", pid, comm);
                struct pid_list *new_pid = (struct pid_list *)malloc(sizeof(struct pid_list));
                struct pid_list *tmp_pid = (struct pid_list *)malloc(sizeof(struct pid_list));
                if (new_pid == NULL)
                    continue;
                if (tmp_pid == NULL) {
                    free(new_pid);
                    continue;
                }
                new_pid->pid = pid;
                new_pid->next = NULL;
                tmp_pid->pid = pid;
                tmp_pid->next = NULL;
                if (insert_co_pid_2_list(&co_pid_list, new_pid) == SUCCESS) {
                    /* we will use parent_co_pids to find the child processes of 
                    co-processes we just found */
                    if (insert_co_pid_2_list(&parent_co_pids, tmp_pid) == ERROR)
                        free(tmp_pid);
                }
                else
                    free(new_pid);
            }
        }
        closedir(d);
        if (parent_co_pids == NULL)
            return NULL;
        struct pid_list *cur = parent_co_pids;
        struct pid_list *prev = NULL;
        while(cur!=NULL) {
            struct pid_list *child_co_pids = find_co_pid(cur->pid, test_type);
            for(struct pid_list *tmp=child_co_pids; tmp!=NULL;) {
                struct pid_list *new_pid_node = (struct pid_list *)malloc(sizeof(struct pid_list));
                new_pid_node->pid = tmp->pid;
                new_pid_node->next = NULL;
                TESTER_LOG(DBG, NULL, test_type, "inserting pid %d to list", tmp->pid);
                if (insert_co_pid_2_list(&co_pid_list, new_pid_node) == ERROR)
                    TESTER_LOG(DBG, NULL, test_type, "insert pid %d to list failed", tmp->pid);
                struct pid_list *tmp_prev = tmp;
                tmp=tmp->next;
                free(tmp_prev);
            }
            prev = cur;
            cur = cur->next;
            free(prev);
        }
        TESTER_LOG(DBG, NULL, test_type, "finish finding pid %d's co pid", parent_pid);

        return co_pid_list;
    }

    return NULL;
}

void close_logfile(struct thread_list *target_thread)
{
    if (strlen(target_thread->log_info.logfile_proc_path) != 0) {
        struct stat statbuf;
        if (stat(target_thread->log_info.logfile_proc_path, &statbuf) == 0) {
            if (target_thread->log_info.log_fp != NULL) {
                fclose(target_thread->log_info.log_fp);
                target_thread->log_info.log_fp = NULL;
            }
        }
    }
}

BOOL is_process_exist(pid_t pid)
{
    if (kill(pid, 0) == 0)
        return TRUE;
    return FALSE;
}

void kill_co_process(struct thread_list *target_thread)
{
    struct pid_list *prev = NULL;
    TEST_TYPE test_type = target_thread->test_type;
    for(struct pid_list *cur=target_thread->pid_list; cur!=NULL;) {
        if (cur->pid == 0)
            continue;
        if (is_process_exist(cur->pid)) {
            TESTER_LOG(DBG, NULL, test_type, "kill co pids = %d", cur->pid);
            kill(cur->pid, SIGKILL);
        }
        prev = cur;
        cur=cur->next;
        free(prev);
    }
    target_thread->pid_list = NULL;
}
