#include "types_and_constants.h"


SOCKET get_listening_conn_sock();

chat_socks_t* get_chat_socks();

BOOL is_place_for_client(chat_socks_t* chat_socks);

int get_free_client_id(chat_socks_t* chat_socks);

receive_from_client_params_t* get_receive_from_client_params(SOCKET chat_sock, int client_id, chat_socks_t* chat_socks);

char* get_ip_and_port_string(sockaddr_in address);


int add_client(chat_socks_t* chat_socks, SOCKET chat_sock, sockaddr_in cli_addr);

void add_client_thread(chat_socks_t* chat_socks, int client_id, HANDLE client_thread);

void remove_client(chat_socks_t* chat_socks, int client_id);


void connect_clients(SOCKET conn_sock, chat_socks_t* chat_socks);

DWORD WINAPI receive_from_client(receive_from_client_params_t* params);

void send_to_clients(char message_buffer[], chat_socks_t* chat_socks, int sending_client_id);

DWORD WINAPI check_connection(chat_socks_t* chat_socks);