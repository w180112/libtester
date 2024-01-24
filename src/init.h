#ifndef _INIT_H_
#define _INIT_H_

#include <common.h>

extern STATUS libtester_ipc_init(void);
extern STATUS libtester_init(tIPC_ID *q_key, char *logfile_path, FILE **log_fp);
extern void libtester_bye(int signal_num);
extern void libtester_clean();

extern tIPC_ID q_key;

typedef struct {
    U16  type;
    U8   refp[1500];
    int	 len;
} tTESTER_MBX;

#endif