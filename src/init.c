#include <common.h>
#include <signal.h>
#include "tester.h"
#include "thread.h"
#include "dbg.h"

#define shell_tester_Q_KEY 0x0b00

tIPC_ID shell_tester_qid=-1;
tIPC_ID q_key;
pid_t 	tmr_pid;

/*---------------------------------------------------------
 * shell_tester_bye : signal handler for INTR-C only
 *--------------------------------------------------------*/
void shell_tester_bye(int signal_num, siginfo_t *info, void *context)
{
	(void)(context);

	TESTER_LOG(INFO, NULL, "recv signal %d, code %d", signal_num, info->si_code);
    TESTER_LOG(INFO, NULL, "delete Qid(0x%x)",shell_tester_qid);
    DEL_MSGQ(shell_tester_qid);
	tmrExit();
    TESTER_LOG(INFO, NULL, "bye!");
	_exit(0);
}

/*---------------------------------------------------------
 * shell_tester_ipc_init
 *--------------------------------------------------------*/
STATUS shell_tester_ipc_init(FILE *log_fp)
{
	if (shell_tester_qid != -1){
		TESTER_LOG(INFO, log_fp, "Qid(0x%x) is already existing",shell_tester_qid);
		return SUCCESS;
	}
	
	if (GET_MSGQ(&shell_tester_qid,shell_tester_Q_KEY) == ERROR){
	   	TESTER_LOG(INFO, log_fp, "can not create a msgQ for key(0x0%x)",shell_tester_Q_KEY);
	   	return ERROR;
	}
	TESTER_LOG(INFO, log_fp, "new Qid(0x%x)",shell_tester_qid);
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
        act.sa_sigaction = shell_tester_bye;
        sigaction(exit_signals[i], &act, NULL);
    }

	const int ignore_signals[] = {
		SIGHUP,
	};
	for(int i=0; i<sizeof(ignore_signals)/sizeof(ignore_signals[0]); i++)
        signal(ignore_signals[i], SIG_IGN);
}

/**************************************************************
 * tester_init: 
 *
 **************************************************************/
int shell_tester_init(tIPC_ID *q_key, FILE *log_fp)
{	
	shell_tester_ipc_init(log_fp);
    *q_key = shell_tester_qid;
    tmr_pid = tmrInit();
	register_signal();
	TESTER_LOG(INFO, log_fp, "%s", "============ shell tester init successfully ==============");

	return 0;
}
