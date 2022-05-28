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

// Sending and receiving an entire file.
int receive_upload(client_socket){
    int received_size;
    char destination_path[] = "./Local Directory/received_file.jpg";  // Note how we don't have the original file name.
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
        printf("Client: received %i bytes from server.\n", received_size);

        // The server has closed the connection.
        // Note: the server will only close the connection when the application terminates.
        if (received_size == 0){
            close(client_socket);
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
	char cmd[1];
	char okay[1];
	int has_cmd = 0;
    while (1){  // We go into an infinite loop because we don't know how many messages we are going to receive.
		if (!has_cmd)
		{
			int received_size = recv(client_socket, cmd, 1, 0);
			if (received_size == 0)
			{
				close(client_socket);
				break;
			}
			printf("server: received server code: %c\n", cmd[0]);
			okay[0] = 'K';
			send(client_socket, okay, 1, 0);
			has_cmd = 1;
		}
		else
		{
			if (cmd[0] == 'u')
			{
				int received_size =recv(client_socket, buffer, 1024, 0);
				if (received_size == 0)
				{
					close(client_socket);
					has_cmd = 0;
					break;
				}
				printf("server: received message: %s\n", buffer);
				bzero(buffer, 1024);
				okay[0] = 'K';
				send(client_socket, okay, 1, 0);
			}
		}
		
		/*while(1)
		{
			printf("before int received_size\n");
			int received_size = recv(client_socket, buffer, 1024, 0);
        	if (received_size == 0){  // Socket is closed by the other end.
            	close(client_socket);
            	//close(server_socket);
            	break;
       		}
		}*/
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
