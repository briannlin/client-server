// The 'server.c' code goes here.
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Server Domain'.
#define PORT 9999

// Sending and receiving multiple messages message.
int server_process(client_socket, server_socket){
    char buffer[1024];
    while (1){  // We go into an infinite loop because we don't know how many messages we are going to receive.
        int received_size = recv(client_socket, buffer, 1024, 0);
        if (received_size == 0){  // Socket is closed by the other end.
            close(client_socket);
            //close(server_socket);
            break;
        }
        printf("Received %s from client with size = %i\n", buffer, received_size);
        bzero(buffer, 1024);
    }
    return 0;
}

int start_server(char* ip_address)
{
    int client_socket, server_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);


    // Creating socket file descriptor
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int socket_status = setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR , &opt, sizeof(opt));
    if (socket_status) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
//    address.sin_addr.s_addr = INADDR_ANY;
    // The server IP address should be supplied as an argument when running the application.
    address.sin_addr.s_addr = inet_addr(ip_address);
    address.sin_port = htons(PORT);

    int bind_status = bind(server_socket, (struct sockaddr*)&address, sizeof(address));
    if (bind_status < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
	printf("server binded\n");
    int listen_status = listen(server_socket, 1);
    if (listen_status < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
	while (1)
	{
		printf("server listening\n");
		client_socket = accept(server_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
		if (client_socket < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}
		printf("server accepted client\n");

		///////////// Start sending and receiving process //////////////
		server_process(client_socket, server_socket);
	}
	// TODO: CLOSE SOCKETS WHEN CTRL C - SIGINT?
    return 0;

}


int main(int argc, char *argv[])
{
	char* ip_address = argv[1];
	printf("I am the server.\n");
	printf("Server IP address: %s\n", ip_address); // TODO: read ip addr from cmdline
	md5_print();
	printf("-----------\n");
	
	start_server(ip_address);

	return 0;
}
