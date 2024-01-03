#include <common.h>
#include <errno.h>
#include <signal.h>
#include "thread.h"
#include "dbg.h"

#define libtester_Q_KEY 0x0b00

tIPC_ID libtester_qid=-1;
tIPC_ID q_key;
pid_t 	tmr_pid;

void libtester_clean()
{
    TESTER_LOG(INFO, NULL, 0, "delete Qid(0x%x)", libtester_qid);
    DEL_MSGQ(libtester_qid);
	tmrExit();
}

/*---------------------------------------------------------
 * libtester_bye : signal handler for INTR-C only
 *--------------------------------------------------------*/
void libtester_bye(int signal_num, siginfo_t *info, void *context)
{
	(void)(context);

	TESTER_LOG(INFO, NULL, 0, "recv signal %d, code %d", signal_num, info->si_code);
    libtester_clean();
	TESTER_LOG(INFO, NULL, 0, "bye!");
	killpg(getpgrp(), SIGKILL);
	//_exit(0);
}

/*---------------------------------------------------------
 * libtester_ipc_init
 *--------------------------------------------------------*/
STATUS libtester_ipc_init(FILE *log_fp)
{
	if (libtester_qid != -1){
		TESTER_LOG(INFO, log_fp, 0, "Qid(0x%x) is already existing", libtester_qid);
		return SUCCESS;
	}
	
	if (GET_MSGQ(&libtester_qid, libtester_Q_KEY) == ERROR){
	   	TESTER_LOG(INFO, log_fp, 0, "can not create a msgQ for key(0x0%x)", libtester_Q_KEY);
	   	return ERROR;
	}
	TESTER_LOG(INFO, log_fp, 0, "new Qid(0x%x)", libtester_qid);
	return SUCCESS;
}

void register_signal()
{ 
    struct sigaction act;

    const int exit_signals[] = {
    	SIGINT,
		SIGQUIT,
    	SIGILL,
    	SIGFPE,
    	SIGBUS,
    	SIGSEGV,
    	SIGTERM,
    	SIGABRT,
		SIGPIPE,
    };
    memset(&act, 0, sizeof(act));
    for(int i=0; i<sizeof(exit_signals)/sizeof(exit_signals[0]); i++) {
        act.sa_flags = SA_SIGINFO;
        act.sa_sigaction = libtester_bye;
        sigaction(exit_signals[i], &act, NULL);
    }

	const int ignore_signals[] = {
		SIGHUP,
	};
	for(int i=0; i<sizeof(ignore_signals)/sizeof(ignore_signals[0]); i++)
        signal(ignore_signals[i], SIG_IGN);
}

/**************************************************************
 * libtester_init: 
 *
 **************************************************************/
STATUS libtester_init(tIPC_ID *q_key, char *logfile_path, FILE **log_fp)
{
	char pwd[PATH_MAX];
    char *logfile_name = "/libtester.log";
    strncpy(pwd, logfile_path, PATH_MAX-strlen(logfile_name)-1);
    pwd[PATH_MAX-strlen(logfile_name)-1] = '\0';

    *log_fp = fopen(strncat(pwd, logfile_name, strlen(logfile_name)+1), "w");
    if (*log_fp == NULL) {
        TESTER_LOG(INFO, NULL, 0, "open libtester logfile failed: %s", strerror(errno));
        return ERROR;
    }

	if (libtester_ipc_init(*log_fp) == ERROR) {
		TESTER_LOG(INFO, *log_fp, 0, "ipc init failed: %s", strerror(errno));
		fclose(*log_fp);
		return ERROR;
	}
    *q_key = libtester_qid;
    tmr_pid = tmrInit();
	if (tmr_pid < 0) {
		TESTER_LOG(INFO, *log_fp, 0, "timer init failed: %s", strerror(errno));
		fclose(*log_fp);
		return ERROR;
	}
	register_signal();
	TESTER_LOG(INFO, *log_fp, 0, "%s", "============ tester init successfully ==============");

	return SUCCESS;
}
