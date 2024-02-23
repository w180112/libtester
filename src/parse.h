#ifndef _PARSE_H_
#define _PARSE_H_

#include <common.h>

struct cmd_opt {
    char default_script_path[PATH_MAX];
    char service_logfile_path[PATH_MAX];
    char service_config_path[PATH_MAX];
};

struct cfg_opt {
    int sock_port;
    int loglvl;
};

int parse_cmd(int argc, char **argv, struct cmd_opt *options);
STATUS parse_config(const char *config_path);

#endif