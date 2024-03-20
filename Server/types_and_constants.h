#include <stdlib.h>
#include <stdio.h>
#include <windows.h>
#include <winsock.h>


#define NAN -1

#define SERV_PORT 1030

#define MESSAGES_QUEUE_SIZE 64
#define MESSAGES_BUFFER_SIZE 256
#define MAX_CLIENTS_N 5
#define MAX_MESSAGES_QUEUED_N 5

#define CONN_CHECK_STR "-"
#define CONN_CHECK_INTERVAL 1000


typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct chat_socks_t
{
	int last_sock;
	SOCKET socks[MAX_CLIENTS_N];

	int clients_ids[MAX_CLIENTS_N];
	char* clients_ips_and_ports[MAX_CLIENTS_N];

	HANDLE client_threads[MAX_CLIENTS_N];

	CRITICAL_SECTION crit_sect;
}
chat_socks_t;

typedef struct receive_from_client_params_t
{
	SOCKET chat_sock;
	int client_id;

	chat_socks_t* chat_socks;
}
receive_from_client_params_t;