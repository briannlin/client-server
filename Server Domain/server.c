// The 'server.c' code goes here.
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Server Domain'.
#define PORT 9999
#define MAXARGS 5
#define MAXLINE 100
#define MAX_THREADS 100
#define MAX_FILES 100


pthread_t client_threads[MAX_THREADS];

struct ServerFile
{
	int id;
	char* filename;
	int locked;
};
struct ServerFile server_files[MAX_FILES];


void lock(char* filename)
{
	for (int i = 0; i < MAX_FILES; i++)
	{
		 if (server_files[i].id == -1)
		 {
			// If trying to lock a file that doesn't have an entry in server_files, create a new locked ServerFile
			server_files[i].id = i;
			char* filename_str = malloc(MAXLINE+1);
            strcpy(filename_str, filename);
			server_files[i].filename = filename_str;
			server_files[i].locked = 1;
			return;
		 }
		 else if (strcmp(server_files[i].filename, filename) == 0)
		 {
			 // Found file, lock it
			 server_files[i].locked = 1;
			 return;
		 }
	}
}

void unlock(char* filename)
{
	for (int i = 0; i < MAX_FILES; i++)
	{
		 if (server_files[i].id == -1)
		 {
			// If trying to unlock a file that doesn't have an entry in server_files, create a new unlocked ServerFile
			server_files[i].id = i;
			char* filename_str = malloc(MAXLINE+1);
            strcpy(filename_str, filename);
			server_files[i].filename = filename_str;
			server_files[i].locked = 0;
			return;
		 }
		 else if (strcmp(server_files[i].filename, filename) == 0)
		 {
			 // Found file, unlock it
			 server_files[i].locked = 0;
			 return;
		 }
	}
}

int lockStatus(char* filename)
{
	// Locked: 1
	// Unlocked: 0 
	for (int i = 0; i < MAX_FILES; i++)
	{
		if (server_files[i].id == -1)
		{
			// File hasn't been accessed by any client, so it must not have been appended to ever, so it must
			// be in an unlocked state.
			return 0;
		}
		else if (strcmp(server_files[i].filename, filename) == 0)
		{
			// Found file, return its lock status
			return server_files[i].locked;
		}
	}
	return 0;
}


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
void receive_upload(int client_socket, char* filename){
	if (lockStatus(filename) == 1)
	{
		printf("File %s is locked, cannot receive upload\n", filename);
		send_ok('A', client_socket);
		return;
	}
	else
	{
		send_ok('K', client_socket);
	}

    int received_size;
	int total_bytes = 0;

	char destination_path[100];
	strcpy(destination_path, "./Remote Directory/");
	strcat(destination_path, filename);
	
    int chunk_size = 1000;

    char file_chunk[chunk_size];
//    int chunk_counter = 0;

    FILE *fptr;

    // Opening a new file in write-binary mode to write the received file bytes into the disk using fptr.
    fptr = fopen(destination_path,"wb");

    // Receive the expected file size to be uploaded.
	char file_size_str[80];
	recv(client_socket, file_size_str, 80, 0);
	int file_size = atoi(file_size_str);
	printf("Expected file size: %i\n", file_size);
	send_ok('K', client_socket);

    // Keep receiving bytes until we receive the whole file.
    while (total_bytes < file_size){
        bzero(file_chunk, chunk_size);

        // Receiving bytes from the socket.
        received_size = recv(client_socket, file_chunk, chunk_size, 0);
		send_ok('K', client_socket);
		total_bytes += received_size;

        // Writing the received bytes into disk.
        fwrite(&file_chunk, sizeof(char), received_size, fptr);
    }
	fclose(fptr);
	printf("%i bytes downloaded successfully.\n", total_bytes);
}

void send_download(int client_socket, char* filename)
{
	if (lockStatus(filename) == 1)
	{
		printf("File %s is locked, cannot download\n", filename);
		send_ok('A', client_socket);
		return;
	}

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

        // Send to server the expected file size it should receive.
        char file_size_str[80];
   		sprintf(file_size_str, "%i", file_size);
		send(client_socket, file_size_str, 80, 0);
		receive_ok(client_socket);

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

		}
		fclose(fptr);
		printf("%i bytes uploaded to local successfully.\n", total_bytes);
	}
}

void delete_file(int client_socket, char* filename)
{
	if (lockStatus(filename) == 1)
	{
		printf("File %s is locked, cannot delete\n", filename);
		send_ok('A', client_socket);
		return;
	}

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
	if (lockStatus(filename) == 1)
	{
		printf("File %s is locked, cannot append\n", filename);
		send_ok('A', client_socket);
		return;
	}
	
	// Try and open file: if successful, then change into append mode (loop). 
	// If not successful (file DNE), return N to client
	// If file locked, return A to client
	char destination_path[100];
	strcpy(destination_path, "./Remote Directory/");
	strcat(destination_path, filename);
	printf("source path: %s\n", destination_path);
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
		printf("Locking file %s\n", filename);
		lock(filename);
		fptr = fopen(destination_path, "ab");
		send_ok('K', client_socket);
		char line_buf[MAXLINE];
		while (1)
		{
			recv(client_socket, line_buf, MAXLINE, 0);
			if (strcmp(line_buf, "close") == 0)
			{
				printf("Unlocking and closing file %s\n", filename);
				unlock(filename);
				fclose(fptr);
				break;
			}
			else
			{
				printf("Appending \"%s\" to %s\n", line_buf, filename);
				fprintf(fptr, "\n%s", line_buf);
				fflush(fptr);
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
		send(client_socket, md5_buf, 17, 0);			// send md5 hash of remote file
		receive_ok(client_socket);

		char lock_status[1];
		if (lockStatus(filename))
		{
			lock_status[0] = '1';
		}
		else
		{
			lock_status[0] = '0';
		}
		printf("sending lockstatus %c\n", lock_status[0]);
		send(client_socket, lock_status, 1, 0);
		receive_ok(client_socket);
	}
}


// Sending and receiving multiple messages message.
void* server_process(void* vargp){
	int client_socket = (int)vargp;
    char buffer[1024];

    while (1){  // We go into an infinite loop because we don't know how many messages we are going to receive.
		// If server is not currently handling a client's command, then the next thing 
		// received must be a new client command, or 0 (socket closed).
		int received_size = recv(client_socket, buffer, 1024, 0);
		if (received_size == 0)
		{
			// TODO: Client closes connection entirely (quit cmd). free thread or smt?
			// TODO: flush buffer, okay, filepath
			close(client_socket);
			pthread_exit(NULL);
			break;
		}

		printf(">server: received command: %s\n", buffer);
		char** tokens = tokenize(buffer);
		char* command = tokens[0];
		if (strcmp(command, "upload") == 0)
		{
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
        perror("server bind failed");
        exit(EXIT_FAILURE);
    }
	printf("server binded\n");
    int listen_status = listen(server_socket, 1);
    if (listen_status < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

	int i = 0;
	while (1)
	{
		printf("\nserver waiting to accept\n");
		client_socket = accept(server_socket, (struct sockaddr*)&address, (socklen_t*)&addrlen);
		if (client_socket < 0) {
			perror("accept");
			exit(EXIT_FAILURE);
		}

		// Create separate thread to handle client request
		if (pthread_create(&client_threads[i++], NULL, server_process, (void *)client_socket) != 0)
		{
			// Error in creating thread
            printf("Failed to create thread\n");
		}
		else
		{
			printf("server accepted client and created thread\n");
		}

		///////////// Start sending and receiving process //////////////
	}
    return 0;

}


int main(int argc, char *argv[])
{
	// Initialize server file array to be null
	for (int i = 0; i < MAX_FILES; i++)
    {
        server_files[i].id = -1;
    }

	char* ip_address = argv[1];
	printf("I am the server.\n");
	printf("Server IP address: %s\n", ip_address); // TODO: read ip addr from cmdline
	printf("-----------\n");
	start_server(ip_address);

	return 0;
}
