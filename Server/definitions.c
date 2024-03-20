#include "headers.h"


SOCKET get_listening_conn_sock()
{
	SOCKET conn_sock = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in serv_conn_addr;
	memset((void*)(&serv_conn_addr), 0, sizeof(serv_conn_addr));
	serv_conn_addr.sin_family = AF_INET;
	serv_conn_addr.sin_port = htons(SERV_PORT);
	serv_conn_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	int result = bind(conn_sock, (sockaddr FAR*) & serv_conn_addr, sizeof(serv_conn_addr));
	if (result == 0)
		result = listen(conn_sock, MAX_MESSAGES_QUEUED_N);

	return conn_sock;
}

chat_socks_t* get_chat_socks()
{
	chat_socks_t* chat_socks = (chat_socks_t*)malloc(sizeof(chat_socks_t));
	
	for (int i = 0; i < MAX_CLIENTS_N; i++)
	{
		chat_socks->socks[i] = NULL;
		chat_socks->clients_ids[i] = NAN;
	}
	chat_socks->last_sock = NAN;

	InitializeCriticalSection(&chat_socks->crit_sect);

	return chat_socks;
}

BOOL is_place_for_client(chat_socks_t* chat_socks)
{
	for (int i = 0; i < MAX_CLIENTS_N; i++)
	{
		if (chat_socks->clients_ids[i] == NAN)
			return TRUE;
	}

	return FALSE;
}

int get_free_client_id(chat_socks_t* chat_socks)
{
	int client_id = 0;
	BOOL found = FALSE;

	while (!found)
	{
		found = TRUE;

		for (int i = 0; i < MAX_CLIENTS_N; i++)
		{
			if (chat_socks->clients_ids[i] == client_id)
			{
				client_id++;
				found = FALSE;
				break;
			}
		}
	}

	return client_id;
}

receive_from_client_params_t* get_receive_from_client_params(SOCKET chat_sock, int client_id, chat_socks_t* chat_socks)
{
	receive_from_client_params_t* receive_from_client_params =
		(receive_from_client_params_t*)malloc(sizeof(receive_from_client_params_t));
	
	receive_from_client_params->chat_sock = chat_sock;
	receive_from_client_params->client_id = client_id;
	receive_from_client_params->chat_socks = chat_socks;

	return receive_from_client_params;
}

char* get_ip_and_port_string(sockaddr_in address)
{
	char* ip = inet_ntoa(address.sin_addr);
	u_short port = ntohs(address.sin_port);

	const int ip_str_len = strlen(ip);
	const int port_str_len = 5;
	const int additional_chars_len = 2;

	char* ip_and_port = (char*)malloc((ip_str_len + port_str_len + additional_chars_len) * sizeof(char));
	snprintf(ip_and_port, ip_str_len + port_str_len + additional_chars_len, "%s:%d", ip, port);

	return ip_and_port;
}


int add_client(chat_socks_t* chat_socks, SOCKET chat_sock, sockaddr_in cli_addr)
{
	int client_id = NAN;

	EnterCriticalSection(&chat_socks->crit_sect);

	if (is_place_for_client(chat_socks))
	{
		chat_socks->last_sock = (chat_socks->last_sock + 1) % MAX_CLIENTS_N;
		chat_socks->socks[chat_socks->last_sock] = chat_sock;

		client_id = get_free_client_id(chat_socks);
		chat_socks->clients_ids[chat_socks->last_sock] = client_id;

		chat_socks->clients_ips_and_ports[chat_socks->last_sock] = get_ip_and_port_string(cli_addr);
	}

	LeaveCriticalSection(&chat_socks->crit_sect);

	return client_id;
}

void add_client_thread(chat_socks_t* chat_socks, int client_id, HANDLE client_thread)
{
	EnterCriticalSection(&chat_socks->crit_sect);

	chat_socks->client_threads[client_id] = client_thread;

	LeaveCriticalSection(&chat_socks->crit_sect);
}

void remove_client(chat_socks_t* chat_socks, int client_id)
{
	EnterCriticalSection(&chat_socks->crit_sect);

	for (int i = 0; i < MAX_CLIENTS_N; i++)
	{
		if (chat_socks->clients_ids[i] == client_id)
		{
			chat_socks->clients_ids[i] = NAN;
			chat_socks->socks[i] = NULL;
			chat_socks->last_sock--;

			int thread_id = GetThreadId(chat_socks->client_threads[i]);
			printf("Terminating thread: %d\n", thread_id);

			TerminateThread(chat_socks->client_threads[i], 0);
			chat_socks->client_threads[i] = NULL;

			free(chat_socks->clients_ips_and_ports[i]);
		}
	}

	LeaveCriticalSection(&chat_socks->crit_sect);
}


void connect_clients(SOCKET conn_sock, chat_socks_t* chat_socks)
{
	SOCKET chat_sock;
	sockaddr_in cli_addr;
	int lenc = sizeof(cli_addr);

	while (TRUE)
	{
		chat_sock = accept(conn_sock, (sockaddr FAR*) & cli_addr, &lenc);
		getpeername(chat_sock, (sockaddr*)&cli_addr, &lenc);
		
		int client_id = add_client(chat_socks, chat_sock, cli_addr);

		char response[MESSAGES_BUFFER_SIZE];
		memset(response, '\0', MESSAGES_BUFFER_SIZE);

		if (client_id != NAN)
		{			
			printf("Connected client: %s\n", chat_socks->clients_ips_and_ports[client_id]);

			snprintf(response, MESSAGES_BUFFER_SIZE, "Connected");

			receive_from_client_params_t* receive_from_client_params =
				get_receive_from_client_params(chat_sock, client_id, chat_socks);
			HANDLE client_thread =
				CreateThread(NULL, NULL, receive_from_client, receive_from_client_params, NULL, NULL);
			add_client_thread(chat_socks, client_id, client_thread);
			
			int client_thread_id = GetThreadId(client_thread);
			printf("Started thread: %d\n", client_thread_id);
		}
		else
		{
			snprintf(response, MESSAGES_BUFFER_SIZE, "Not connected");
		}

		send(chat_sock, response, sizeof(response), 0);
	}
}

DWORD WINAPI receive_from_client(receive_from_client_params_t* params)
{
	char message_buffer[MESSAGES_BUFFER_SIZE];
	memset(message_buffer, '\0', MESSAGES_BUFFER_SIZE);

	char named_message_buffer[MESSAGES_BUFFER_SIZE];

	while (recv(params->chat_sock, message_buffer, MESSAGES_BUFFER_SIZE, 0) > 0)
	{
		memset(named_message_buffer, '\0', MESSAGES_BUFFER_SIZE);
		snprintf(named_message_buffer, MESSAGES_BUFFER_SIZE, "[%s] %s",
			params->chat_socks->clients_ips_and_ports[params->client_id], message_buffer);

		printf("%s\n", named_message_buffer);
		send_to_clients(named_message_buffer, params->chat_socks, params->client_id);
		memset(message_buffer, '\0', MESSAGES_BUFFER_SIZE);
	}
}

void send_to_clients(char message_buffer[], chat_socks_t* chat_socks, int sending_client_id)
{
	for (int i = 0; i < MAX_CLIENTS_N; i++)
	{
		if (chat_socks->clients_ids[i] != NAN && chat_socks->clients_ids[i] != sending_client_id)
		{
			send(chat_socks->socks[i], message_buffer, strlen(message_buffer), 0);
		}
	}
}

DWORD WINAPI check_connection(chat_socks_t* chat_socks)
{
	while (TRUE)
	{
		for (int i = 0; i < MAX_CLIENTS_N; i++)
		{
			if (chat_socks->clients_ids[i] != NAN)
			{
				int sent = send(chat_socks->socks[i], CONN_CHECK_STR, 1, 0);

				if (sent < 0)
				{
					remove_client(chat_socks, chat_socks->clients_ids[i]);
				}
			}
		}

		Sleep(CONN_CHECK_INTERVAL);
	}
}