/*\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
  DBG.H
  Designed by Dennis Tseng on Jan 1, 2002
/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\*/

#ifndef _DBG_H_
#define _DBG_H_

#include <common.h>

#define	LOGDBG		1U
#define LOGINFO		2U

/* log level, logfile fp, log msg */
#define TESTER_LOG(lvl, fp, ...) LOGGER(LOG ## lvl, __FILE__, __LINE__, fp, __VA_ARGS__)

extern void LOGGER(U8 level, char *filename, int line_num, FILE *log_fp, char *fmt,...);
extern char *loglvl2str(U8 level);
extern U8 tester_dbg_flag;

#endif