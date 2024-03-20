#include "headers.h"


int main()
{
	int result;

	WSADATA wsas;
	WORD version = MAKEWORD(2, 2);
	result = WSAStartup(version, &wsas);

	SOCKET conn_sock = get_listening_conn_sock();
	chat_socks_t* chat_socks = get_chat_socks();

	HANDLE check_connection_thread =
		CreateThread(NULL, NULL, check_connection, chat_socks, NULL, NULL);
	connect_clients(conn_sock, chat_socks);

	TerminateThread(check_connection_thread, 0);
	return 0;
}