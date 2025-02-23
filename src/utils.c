#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <common.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h> 

#include "dbg.h"
#include "utils.h"

char *trim_string(char *str)
{
    char *end;

    while(isspace((unsigned char)*str)) 
        str++;

    if (*str == 0)
        return str;

    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) 
        end--;

    end[1] = '\0';

    return str;
}

FILE *create_logfile(TEST_TYPE test_type, char cwd[], char logfile_proc_path[])
{
    char *test_type_str = total_test_types[test_type-1];
    struct timeval tv;
    char pwd[PATH_MAX];
    memset(pwd, 0, PATH_MAX);
    strncpy(pwd, cwd, PATH_MAX-1);
    pwd[PATH_MAX-1] = '\0';
 
    gettimeofday(&tv, NULL);
    time_t t = tv.tv_sec;
    struct tm *time_format = localtime(&t);

    char logfile_name[192];
    sprintf(logfile_name, "/%s_%d-%d-%d_%d-%d-%d_%ld.log", test_type_str, time_format->tm_year+1900, 
        time_format->tm_mon+1, time_format->tm_mday, time_format->tm_hour, time_format->tm_min, time_format->tm_sec, tv.tv_usec);
    strncat(pwd, logfile_name, PATH_MAX-1);
    FILE *log_fp = fopen(pwd, "w");
    if (log_fp == NULL) {
        TESTER_LOG(INFO, log_fp, test_type, "create logfile [%s] failed: %s", pwd, strerror(errno));
        return NULL;
    }
    TESTER_LOG(DBG, log_fp, test_type, "create log file [%s] for test", pwd);

    pid_t tester_pid = getpid();
    char proc_path[PROC_PATH_LEN];
    memset(proc_path, 0, PROC_PATH_LEN);
    snprintf(proc_path, PROC_PATH_LEN-1, "/proc/%d/fd/", tester_pid);
    proc_path[PROC_PATH_LEN-1] = '\0';
    DIR *d = opendir(proc_path);
    if (d) {
        struct dirent *dir;
        while ((dir = readdir(d)) != NULL) {
            char symlink_real_path[PATH_MAX];
            if (dir->d_type != DT_LNK)
                continue;
            memset(proc_path, 0, PROC_PATH_LEN);
            if (snprintf(proc_path, PROC_PATH_LEN-1, "/proc/%d/fd/%s", tester_pid, dir->d_name) < 0 ) {
                TESTER_LOG(INFO, log_fp, test_type, "get proc fd path %s with snprintf failed", proc_path);
                continue;
            }
            proc_path[PROC_PATH_LEN-1] = '\0';
            int path_size = readlink(proc_path, symlink_real_path, PATH_MAX-1);
            if (path_size == -1) {
                TESTER_LOG(INFO, log_fp, test_type, "read proc fd symlink %s failed", proc_path);
                continue;
            }
            symlink_real_path[path_size] = '\0';
            if (strstr(symlink_real_path, test_type_str) != NULL) {
                strncpy(logfile_proc_path, proc_path, LOG_PATH_LEN-1);
                logfile_proc_path[LOG_PATH_LEN-1] = '\0';
                break;
            }
        }
        closedir(d);
        if (strlen(logfile_proc_path) == 0) {
            TESTER_LOG(INFO, log_fp, test_type, "can't find %s file symlink in /proc/%d/fd/", logfile_name, tester_pid);
            return NULL;
        }
    }
    else {
        TESTER_LOG(INFO, log_fp, test_type, "opendir() failed: %s", strerror(errno));
        return NULL;
    }

    return log_fp;
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
