/**************************************************************************
 * DBG.C
 *
 * Debug methods
 *
 * Created by Dennis Tseng on Nov 05,'08
 **************************************************************************/

#include <common.h>
#include "dbg.h"

#define LOGGER_VA_MSG_LEN 512
#define LOGGER_BUF_LEN 1024

U8 tester_dbg_flag;

/***************************************************
 * LOGGER:
 ***************************************************/	
void LOGGER(U8 level, char *filename, int line_num, FILE *log_fp, char *fmt,...)
{
	va_list ap; /* points to each unnamed arg in turn */
	char    buf[LOGGER_BUF_LEN], msg[LOGGER_VA_MSG_LEN];
	
	//user offer level must > system requirement
    if (tester_dbg_flag > level)
    	return;
    
    va_start(ap, fmt); /* set ap pointer to 1st unnamed arg */
    vsnprintf(msg, LOGGER_VA_MSG_LEN, fmt, ap);
    sprintf(buf, "Shell tester: %s:%d> ", filename, line_num);
    
  	strncat(buf, msg, strlen(msg)+1);
    va_end(ap);

    buf[strlen(buf)] = '\0';
    fprintf(stdout, "%s\n", buf);
    if (log_fp != NULL) {
        fwrite(buf, sizeof(char), strlen(buf), log_fp);
        char *newline = "\n";
        fwrite(newline, sizeof(char), strlen(newline), log_fp);
        fflush(log_fp);
    }

    return;
}

char *loglvl2str(U8 level)
{
    switch (level) {
    case LOGDBG:
        return "DBG";
    case LOGINFO:
        return "INFO";
    default:
        return "UNKNOWN";
    }
}