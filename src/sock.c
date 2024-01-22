#include <common.h>
#include <errno.h>
#include <picohttpparser.h>
#include <linux/route.h>
#include "thread.h"
#include "init.h"
#include "parse.h"
#include "sock.h"
#include "dbg.h"

char *http_ok_header = "HTTP/1.1 200 OK\r\n\n";
char *http_503_header = "HTTP/1.1 503 \r\n\n";
char *http_fail_header = "HTTP/1.1 500 \r\n\n";
char *http_400_header = "HTTP/1.1 400 \r\n\n";
tIPC_ID msg_qid;
extern struct cfg_opt cfg_opt;

STATUS tester_send2mailbox(U8 *mu, int mulen);
STATUS parse_test_request(struct test_info *test_info, char buf[], int n, FILE *log_fp);
void *recv_cmd(void *arg);
void drv_xmit(U8 *mu, U16 mulen, int sock);
STATUS get_sys_default_if(char *test_if_name);

STATUS init_sock(int *server_socket, FILE *log_fp)
{
    char test_if_name[IF_NAMESIZE];
    char tester_ip[16];
    if (get_sys_default_if(test_if_name) == ERROR) {
        TESTER_LOG(INFO, log_fp, 0, "get system default interface ip addr failed");
        return ERROR;
    }
    if (get_local_ip(tester_ip, test_if_name) == ERROR) {
        TESTER_LOG(INFO, log_fp, 0, "get system default interface ip addr failed");
        return ERROR;
    }
    TESTER_LOG(DBG, NULL, 0, "use interface: %s, ip addr: %s", test_if_name, tester_ip);

    *server_socket = socket(AF_INET, SOCK_STREAM, 0);
        
    if (setsockopt(*server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int)) < 0) {
        TESTER_LOG(INFO, log_fp, 0, "set socket option failed: %s", strerror(errno));
        close(*server_socket);
        return ERROR;
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(cfg_opt.sock_port);
    server_address.sin_addr.s_addr = inet_addr(tester_ip);

    bind(*server_socket, (struct sockaddr *) &server_address, sizeof(server_address));

    int listening = listen(*server_socket, 10);
    if (listening < 0) {
        TESTER_LOG(INFO, log_fp, 0, "socket server listens failed");
        close(*server_socket);
        return ERROR;
    }

    return SUCCESS;
}

STATUS get_sys_default_if(char *default_if)
{
    char if_name[IF_NAMESIZE];
    in_addr_t dest, gateway, mask;
    int flags, refcnt, use, metric, mtu, win, irtt;
    STATUS ret = ERROR;

    FILE *fp = fopen("/proc/net/route", "r");
    if (fp == NULL) {
        TESTER_LOG(INFO, NULL, 0, "fopen /proc/net/route failed: %s", strerror(errno));
        return ERROR;
    }
    if (fscanf(fp, "%*[^\n]\n") < 0) {
        TESTER_LOG(INFO, NULL, 0, "fscanf /proc/net/route failed: %s", strerror(errno));
        fclose(fp);
        return ERROR;
    }
    for (;;) {
        int nread = fscanf(fp, "%15s%X%X%X%d%d%d%X%d%d%d\n",
                           if_name, &dest, &gateway, &flags, &refcnt, &use, &metric, &mask, &mtu, &win, &irtt);
        if (nread != 11) {
            break;
        }
        if ((flags & (RTF_UP|RTF_GATEWAY)) == (RTF_UP|RTF_GATEWAY) && dest == 0) {
            strncpy(default_if, if_name, IF_NAMESIZE-1);
            default_if[IF_NAMESIZE-1] = '\0';
            ret = SUCCESS;
            break;
        }
    }
    fclose(fp);
    return ret;
}

void *recv_req(void *arg)
{
    int client_socket;
    msg_qid = *(tIPC_ID *)arg;
    struct sockaddr_in client_addr;
    int server_socket;
    FILE *log_fp = thread_list_head->log_info.log_fp;

    if (init_sock(&server_socket, log_fp) == ERROR) {
        pthread_exit(NULL);
    }

    socklen_t len = sizeof(client_addr);
    while(1) {
        TESTER_LOG(INFO, log_fp, 0, "waiting for test request");
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &len);
        TESTER_LOG(INFO, log_fp, 0, "recv running test request");
        struct thread_list *new_thread = (struct thread_list *)malloc(sizeof(struct thread_list));
        new_thread->sock = client_socket;
        new_thread->log_info.log_fp = thread_list_head->log_info.log_fp;
        new_thread->test_type = -1;
        pthread_create(&new_thread->thread_id, NULL, recv_cmd, (void *restrict)new_thread);
    }
}

void *recv_cmd(void *arg)
{
    struct thread_list *this_thread = (struct thread_list *)arg;
    int client_socket = this_thread->sock;
    FILE *log_fp = this_thread->log_info.log_fp;
    while(1) {
        char buf[1024];
        memset(buf, 0, 1024);
        int n = recv(client_socket, buf, 1024, 0);
        if (n == 0) {
            TESTER_LOG(INFO, log_fp, 0, "client closed\n");
            close(client_socket);
            break;
        }
        else if (n < 0) {
            TESTER_LOG(INFO, log_fp, 0, "socket recv error");
            close(client_socket);
            break;
        }
        struct test_info test_info;
        test_info.socket = client_socket;
        if (parse_test_request(&test_info, buf, n, log_fp) == ERROR) {
            drv_xmit((U8 *)http_400_header, strlen(http_400_header)+1, client_socket);
            continue;
        }
        
        TESTER_LOG(DBG, log_fp, 0, "branch name = %s", test_info.branch_name);
        tester_send2mailbox((U8 *)&test_info, sizeof(test_info));
    }
    free(this_thread);

    pthread_exit(NULL);
}

STATUS parse_test_request(struct test_info *test_info, char buf[], int n, FILE *log_fp)
{
    char *default_test_type = "type1";
    char *default_test_branch = "master";
    strncpy(test_info->test_type, default_test_type, sizeof(test_info->test_type)-1);
    test_info->test_type[sizeof(test_info->test_type)-1] = '\0';
    strncpy(test_info->branch_name, default_test_branch, sizeof(test_info->branch_name)-1);
    test_info->branch_name[sizeof(test_info->test_type)-1] = '\0';

    char *method, *path;
    size_t method_len, path_len, num_headers = 10;
    struct phr_header headers[num_headers];
    int minor_version;
    printf("buf = %s, len = %lu, n = %d\n", buf, strlen(buf), n);
    int ret = phr_parse_request((const char *)buf, n, (const char **)&method, &method_len, (const char **)&path, &path_len,
                             &minor_version, headers, &num_headers, 0);
    if (ret < 0) {
        TESTER_LOG(DBG, NULL, 0, "parse test request http header failed");
        return ERROR;;
    }
    printf("method is %.*s\n", (int)method_len, method);
    printf("path is %.*s\n", (int)path_len, path);
    printf("HTTP version is 1.%d\n", minor_version);
    printf("headers:\n");
    for (int i=0; i<num_headers; i++) {
        printf("%.*s: %.*s\n", (int)headers[i].name_len, headers[i].name, (int)headers[i].value_len, headers[i].value);
        if (strncmp("branch", headers[i].name, headers[i].name_len) == 0) {
            // we can't use strncpy here because the parsed headers may not be contained with '\0' at each header end 
            snprintf(test_info->branch_name, sizeof(test_info->branch_name)-1, "%.*s", (int)headers[i].value_len, headers[i].value);
            test_info->branch_name[sizeof(test_info->branch_name)-1] = '\0';
            continue;
        }
        if (strncmp("test_type", headers[i].name, headers[i].name_len) == 0) {
            snprintf(test_info->test_type, sizeof(test_info->test_type)-1, "%.*s", (int)headers[i].value_len, headers[i].value);
            test_info->test_type[sizeof(test_info->test_type)-1] = '\0';
            continue;
        }
    }

    return SUCCESS;
}

STATUS tester_send2mailbox(U8 *mu, int mulen)
{
	tTESTER_MBX mail;

    if (msg_qid == -1) {
		if ((msg_qid=msgget(0x0b00,0600|IPC_CREAT)) < 0) {
			printf("send> Oops! Q(key=0x%x) not found\n",0x0b00);
   	 	}
	}

	mail.len = mulen;
	memcpy(mail.refp,mu,mulen); /* mail content will be copied into mail queue */
	
	mail.type = IPC_EV_TYPE_DRV;
	ipc_sw(msg_qid, &mail, sizeof(mail), -1);
	return TRUE;
}

void drv_xmit(U8 *mu, U16 mulen, int sock)
{
	if (send(sock, mu, mulen, 0) < 0)
        perror("send error");
}
