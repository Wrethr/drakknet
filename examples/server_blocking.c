#include <stdio.h>
#include "../drakknet.h"

void print_sliceln(const char* str, const char* buffer, 
				size_t from, size_t to);

int main(void) {
	drakknet_error_t err = drakknet_init();
	switch (err) {
	case DRAKKNET_ERR_PLATFORM_INIT:
		fprintf(stderr, "Error: platform init\n");
		return 1;
	default: 
		break;
	}

	drakknet_socket_t server;
	err = drakknet_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP, &server);
	switch (err) {
	case DRAKKNET_ERR_SOCKET_CREATE:
		fprintf(stderr, "Error: sock create\n");
		return 1;
	case DRAKKNET_ERR_INVALID_ARG:
		fprintf(stderr, "Error: sock create invalid arg\n");
		return 1;
	default: 
		break;
	}

	drakknet_addr_t serv_addr;
	err = drakknet_addr_from_string("127.0.0.1", 8080, &serv_addr);
	switch (err) {
	case DRAKKNET_ERR_INVALID_ARG:
		fprintf(stderr, "Error: serv_addr create invalid arg\n");
		return 1;
	default: 
		break;
	}

	err = drakknet_bind(server,  &serv_addr);
	switch (err) {
	case DRAKKNET_ERR_BIND:
		fprintf(stderr, "Error: bind err\n");
		return 1;
	case DRAKKNET_ERR_INVALID_ARG:
		fprintf(stderr, "Error: bind invalid arg\n");
		return 1;
	default: 
		break;
	}

	err = drakknet_listen(server, 1);
	switch (err) {
	case DRAKKNET_ERR_LISTEN:
		fprintf(stderr, "Error: listen\n");
		return 1;
	default:
		break;
	}

	drakknet_socket_t conn;
	drakknet_addr_t client_addr;
	err = drakknet_accept(server, &conn, &client_addr);
	switch (err) {
	case DRAKKNET_ERR_ACCEPT:
		fprintf(stderr, "Error: accept\n");
		return 1;
	case DRAKKNET_ERR_INVALID_ARG:
		fprintf(stderr, "Error: accept invalid arg\n");
		return 1;
	default:
		break;
	}


	char send_buffer[1024] = {};
	size_t send_len = 0;
	
	char recv_buffer[1024] = {};
	size_t recv_len = 0;
	
	while (1) {
		err = drakknet_recv(conn, &recv_buffer, 1024, 0, &recv_len);
		switch (err) {
		case DRAKKNET_ERR_RECV:
			fprintf(stderr, "Error: recv\n");
			return 1;
		case DRAKKNET_ERR_INVALID_ARG:
			fprintf(stderr, "Error: recv invalid arg\n");
			return 1; 
		default: 
			break;
		}

		if (recv_len == 0) {
			printf("User has been closed connection\n");
			break;
		}

		print_sliceln("client: ", recv_buffer, 0, recv_len);
	
		int scanf_clearer;
		int field = scanf("%1023[^\n]", send_buffer);
		if (field != 1) {
			printf("Rewrite your msg\n");
	
			while ((scanf_clearer = getchar()) != '\n' && scanf_clearer != EOF) {}

			continue;
		}
		size_t scanned = strlen(send_buffer);
		
		while ((scanf_clearer = getchar()) != '\n' && scanf_clearer != EOF) {}

		err = drakknet_send(conn, send_buffer, scanned, 0, &send_len);
		switch (err) {
		case DRAKKNET_ERR_SEND:
			fprintf(stderr, "Error: send\n");
			return 1;
		case DRAKKNET_ERR_INVALID_ARG:
			fprintf(stderr, "Error: send invalid arg\n");
			return 1;
		default:
			break;
		}

		if (scanned != send_len) {
			printf("Warning: scanned != send_len on drakknet_send(conn, send, scanned, 0, &send_len);");
		}

		memset(send_buffer, 0, send_len);
		memset(recv_buffer, 0, recv_len);

		send_len = 0;
		recv_len = 0;
	}

	err = drakknet_close(conn);
	switch (err) {
	case DRAKKNET_ERR_UNKNOWN:
		fprintf(stderr, "Error: close conn\n");
		break;
	default:
		break;
	}

	err = drakknet_close(server);
	switch (err) {
	case DRAKKNET_ERR_UNKNOWN:
		fprintf(stderr, "Error: close conn\n");
		break;
	default:
		break;
	}

	drakknet_cleanup();
	return 0;
}

void print_sliceln(const char* str, const char* buffer, 
				size_t from, size_t to) {
	printf("%s", str);
	while (from < to) {
		putchar(buffer[from]);
		from++;
	}
}
