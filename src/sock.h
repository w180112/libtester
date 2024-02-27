#ifndef _SOCK_H_
#define _SOCK_H_

#include <common.h>
#include <uuid/uuid.h>

struct test_info {
	int socket;
	BOOL is_test_end;
	uuid_t test_uuid;
    char test_type[128];
	char branch_name[128];
};

extern char *http_ok_header;
extern char *http_503_header;
extern char *http_fail_header;
extern char *http_400_header;

extern void drv_xmit(U8 *mu, U16 mulen, int sock);
extern void *recv_req(void *arg);

#endif