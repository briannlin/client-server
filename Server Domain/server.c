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
#define MAXARGS 5
#define MAXLINE 100

char** tokenize(char* str)
{
    int i;
    static char *tokens[MAXARGS];
    for (i = 0; i < MAXARGS; i++)
    {
        tokens[i] = NULL;
    }

    i = 0;
    char* token = strtok(str, " \t");
    while (token != NULL)
    {
        tokens[i] = token;
        token = strtok(NULL, " \t");
        i += 1;
    }
    return tokens;
}

// Sending and receiving an entire file.
int receive_upload(int client_socket, char* filename){
    int received_size;

	char destination_path[100];
	strcpy(destination_path, "./Remote Directory/");
	strcat(destination_path, filename);
	
    int chunk_size = 1000;
    char file_chunk[chunk_size];
//    int chunk_counter = 0;

    FILE *fptr;

    // Opening a new file in write-binary mode to write the received file bytes into the disk using fptr.
    fptr = fopen(destination_path,"wb");

    // Keep receiving bytes until we receive the whole file.
    while (1){
        bzero(file_chunk, chunk_size);
//        memset(&file_chunk, 0, chunk_size);

        // Receiving bytes from the socket.
        received_size = recv(client_socket, file_chunk, chunk_size, 0);
        /*printf("Client: received %i bytes from server.\n", received_size);*/

        // The server has closed the connection.
        // Note: the server will only close the connection when the application terminates.
        if (received_size == 0){
            fclose(fptr);
            break;
        }
        // Writing the received bytes into disk.
        fwrite(&file_chunk, sizeof(char), received_size, fptr);
//        printf("Client: file_chunk data is:\n%s\n\n", file_chunk);
    }
    return 0;
}

// Sending and receiving multiple messages message.
int server_process(client_socket, server_socket){
    char buffer[1024];
	char okay[1];
	int cmd = 0;
	FILE* fptr;
    while (1){  // We go into an infinite loop because we don't know how many messages we are going to receive.
		// If server is not currently handling a client's command, then the next thing 
		// received must be a new client command, or 0 (socket closed).
		int received_size = recv(client_socket, buffer, 1024, 0);
		if (received_size == 0)
		{
			// TODO: Client closes connection entirely (quit cmd). free thread or smt?
			// TODO: flush buffer, okay, filepath
			close(client_socket);
			break;
		}

		printf("server: received command: %s\n", buffer);
		okay[0] = 'K';
		send(client_socket, okay, 1, 0);

		char** tokens = tokenize(buffer);
		char* command = tokens[0];
		if (strcmp(command, "upload") == 0)
		{
			cmd = 3;
			printf("filename: %s\n", tokens[1]);
			receive_upload(client_socket, tokens[1]);
			cmd = 0;
		}
		// FLUSH THE BUFFER? PRINTING "upload me.jpgx" (where x coming from?)
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
