#include <common.h>
#include <ctype.h>
#include <libconfig.h>
#include "dbg.h"
#include "parse.h"

struct cfg_opt cfg_opt;

void print_usage(char *argv)
{
    printf("Usage: %s [-dhlsc]\n", argv);
    printf("    -d        sets running in daemon mode to true, default is false\n");
    printf("    -h        show help message\n");
    printf("    -l        set logfile located directory\n");
    printf("    -s        set custom script located directory\n");
    printf("    -c        set config file path\n");
}

int parse_cmd(int argc, char **argv, struct cmd_opt *options)
{
    int opt;
    int ret = 0;
    while ((opt = getopt(argc, argv, "hl:s:c:")) != -1) {
        switch(opt) {
        case 'h':
            print_usage(argv[0]);
            exit(0);
        case 'l':
            strncpy(options->service_logfile_path, optarg, PATH_MAX-1);
            options->service_logfile_path[PATH_MAX-1] = '\0';
            break;
        case 's':
            strncpy(options->default_script_path, optarg, PATH_MAX-1);
            options->default_script_path[PATH_MAX-1] = '\0';
            break;
        case 'c':
            strncpy(options->service_config_path, optarg, PATH_MAX-1);
            options->service_config_path[PATH_MAX-1] = '\0';
            break;
        case '?':
            if (optopt == 's' || optopt == 'l' || optopt == 'c')
                fprintf(stderr, "option -%c requires an argument.\n", optopt);
            else if (isprint(optopt))
                fprintf(stderr, "unknown option -%c\n", optopt);
            else
                fprintf(stderr, "unknown option character \\x%c\n", optopt);
            print_usage(argv[0]);
            return -1;
        default:
            ;
        }
    }
    /*for (int index=optind; index < argc; index++) {
        printf("non-option argument %s\n", argv[index]);
        return -1;
    }*/

    if (optind > 0) {
		argv[optind-1] = argv[0];
	    ret = optind - 1;
    }
	optind = 0; /* reset getopt lib */

    return ret;
}

STATUS parse_config(const char *config_path) 
{
    config_t cfg;

    config_init(&cfg);
    if(!config_read_file(&cfg, config_path)) {
        TESTER_LOG(INFO, NULL, 0, "read config file %s content error: %s:%d - %s", 
                config_path, config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return ERROR;
    }
    if(config_lookup_int(&cfg, "port", &cfg_opt.sock_port) == CONFIG_FALSE)
        cfg_opt.sock_port = 55688;
    if(config_lookup_int(&cfg, "loglvl", &cfg_opt.loglvl) == CONFIG_FALSE)
        cfg_opt.loglvl = LOGDBG;
    
    tester_dbg_flag = cfg_opt.loglvl;
    config_destroy(&cfg);

    return SUCCESS;
}