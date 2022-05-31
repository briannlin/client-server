// The 'server.c' code goes here.
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Server Domain'.
#define PORT 9999
#define MAXARGS 5
#define MAXLINE 100

void send_ok(char k, int client_socket)
{
	char okay[1];
	okay[0] = k;
	send(client_socket, okay, 1, 0);
}

int receive_ok(int client_socket)
{
	char okay[1];
	recv(client_socket, okay, 1, 0);		   // receive OK from server
	return okay[0] == 'K';
}

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

void send_download(int client_socket, char* filename)
{
	FILE *fptr;

	char source_path[100];
	strcpy(source_path, "./Remote Directory/");
	strcat(source_path, filename);

	fptr = fopen(source_path,"rb");  // Open a file in read-binary mode.
	if (!fptr) // If file doesn't exist, say so and return
    {
        printf("File [%s] could not be found in remote directory.\n", filename);
		send_ok('N', client_socket);
    }
	else
	{
		int chunk_size = 1000;
		char file_chunk[chunk_size];

		send_ok('K', client_socket);
		fseek(fptr, 0L, SEEK_END);  // Sets the pointer at the end of the file.
		int file_size = ftell(fptr);  // Get file size.
		fseek(fptr, 0L, SEEK_SET);  // Sets the pointer back to the beginning of the file.

		int total_bytes = 0;  // Keep track of how many bytes we read so far.
		int current_chunk_size;  // Keep track of how many bytes we were able to read from file (helpful for the last chunk).
		ssize_t sent_bytes;

		while (total_bytes < file_size){
			// Clean the memory of previous bytes.
			bzero(file_chunk, chunk_size);

			// Read file bytes from file.
			current_chunk_size = fread(&file_chunk, sizeof(char), chunk_size, fptr);

			// Sending a chunk of file to the socket.
			sent_bytes = send(client_socket, &file_chunk, current_chunk_size, 0);
			receive_ok(client_socket);

			// Keep track of how many bytes we read/sent so far.
			total_bytes = total_bytes + sent_bytes;

			if (file_size == total_bytes)
			{
				send_ok('N', client_socket);
			}
			else
			{
				send_ok('K', client_socket);
			}
		}
		fclose(fptr);
		printf("%i bytes uploaded to local successfully.\n", total_bytes);
	}

}

void delete_file(int client_socket, char* filename)
{
	char source_path[100];
	strcpy(source_path, "./Remote Directory/");
	strcat(source_path, filename);

	if (remove(source_path) == 0)
	{
    	printf("Deleted successfully:%s\n", filename);
		send_ok('K', client_socket);
	}
	else
	{
    	printf("Unable to delete the file / File DNE: %s\n", filename);
		send_ok('N', client_socket);
	}
}

void append_to_file(int client_socket, char* filename)
{

	// Try and open file: if successful, then change into append mode (loop). 
	// If not successful (file DNE), return N to client
	char destination_path[100];
	strcpy(destination_path, "./Remote Directory/");
	strcat(destination_path, filename);

	int fd;
	FILE *fptr;
	fd = open(destination_path, O_APPEND);
	if (fd < 0) 
	{
		printf("File [%s] could not be found in remote directory.\n", filename);
		send_ok('N', client_socket);
	}
	else
	{
		fptr = fopen(destination_path, "ab");
		send_ok('K', client_socket);
		char line_buf[MAXLINE];
		while (1)
		{
			recv(client_socket, line_buf, MAXLINE, 0);
			if (strcmp(line_buf, "close") == 0)
			{
				printf("Closing file %s\n", filename);
				fclose(fptr);
				break;
			}
			else
			{
				printf("Appending \"%s\" to %s\n", line_buf, filename);
				fprintf(fptr, "%s\n", line_buf);
			}
		}
	}
}

void server_syncheck(int client_socket, char* filename)
{
	char source_path[100];
	strcpy(source_path, "./Remote Directory/");
	strcat(source_path, filename);

	struct stat stats;	
	if (stat(source_path, &stats) != 0) // File doesn't exist.
    {
        printf("File [%s] could not be found in remote directory.\n", filename);
		send_ok('N', client_socket);
		receive_ok(client_socket);
    }
	else
	{
		send_ok('K', client_socket);
		receive_ok(client_socket);

		int filesize = stats.st_size;
		printf("File size: %i\n", (int)filesize);
		char filesize_buf[15];
		sprintf(filesize_buf, "%i", filesize);
		send(client_socket, filesize_buf, 15, 0);		// Send size of remote file
		receive_ok(client_socket);

		char md5_buf[17];
		MDFile(source_path, md5_buf);
		md5_buf[16] = 0;
		printf("sending md5 %s\n", md5_buf);
		send(client_socket, md5_buf, 17, 0);
		receive_ok(client_socket);
	}
}


// Sending and receiving multiple messages message.
int server_process(client_socket, server_socket){
    char buffer[1024];
	int a = 0;
	int* append_mode = &a;

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

		printf(">server: received command: %s\n", buffer);
		char** tokens = tokenize(buffer);
		char* command = tokens[0];
		if (strcmp(command, "upload") == 0)
		{
			send_ok('K', client_socket);
			char* filename = tokens[1];
			printf("upload filename: %s\n", filename);
			receive_upload(client_socket, filename);
		}
		else if (strcmp(command, "download") == 0)
		{
			char* filename = tokens[1];
			printf("download filename: %s\n", filename);
			send_download(client_socket, filename);
		}
		else if (strcmp(command, "delete") == 0)
		{
			char* filename = tokens[1];
			printf("delete filename: %s\n", filename);
			delete_file(client_socket, filename);
		}
		else if (strcmp(command, "append") == 0)
		{
			char* filename = tokens[1];
			printf("append filename: %s\n", filename);
			append_to_file(client_socket, filename);
		}
		else if (strcmp(command, "syncheck") == 0)
		{
			char* filename = tokens[1];
			printf("syncheck filename: %s\n", filename);
			server_syncheck(client_socket, filename);
		}
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
		printf("\nserver listening\n");
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
	printf("-----------\n");
	start_server(ip_address);

	return 0;
}
