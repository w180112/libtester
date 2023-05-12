#ifndef _INIT_H_
#define _INIT_H_

#include <common.h>

extern STATUS shell_tester_ipc_init(void);
extern int shell_tester_init(tIPC_ID *q_key, FILE *log_fp);
extern void shell_tester_bye(int signal_num);

extern tIPC_ID q_key;

typedef struct {
	U16  type;
	U8   refp[1500];
	int	 len;
} tTESTER_MBX;

#endif