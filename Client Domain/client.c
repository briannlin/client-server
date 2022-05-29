// The 'client.c' code goes here.
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Client Domain'.
#define PORT 9999
#define MAXLINE 100
#define MAXARGS 3

int start_client(char* user_commands, char* ip_address, int start_process);

int receive_ok(int client_socket)
{
	char okay[1];
	recv(client_socket, okay, 1, 0);		   // receive OK from server
	return okay[0] == 'K';
}

void send_ok(char k, int client_socket)
{
	char okay[1];
	okay[0] = k;
	send(client_socket, okay, 1, 0);
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

void client_upload(int client_socket, char* ip_address, char* command_line, char* filename)
{
	/* upload a file from the local directory to the remote directory*/
	FILE *fptr;
    int chunk_size = 1000;
    char file_chunk[chunk_size];

	char source_path[100];
	strcpy(source_path, "./Local Directory/");
	strcat(source_path, filename);

    fptr = fopen(source_path,"rb");  // Open a file in read-binary mode.
	if (!fptr) // If file doesn't exist, say so and return
    {
        printf("File [%s] could not be found in local directory.\n", filename);
    }
	else
	{
		// Send to server: command
		//char okay[1];
		send(client_socket, command_line, MAXLINE, 0);        // send command to server
		//recv(client_socket, okay, 1, 0);		   // receive OK from server
		receive_ok(client_socket);

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

			// Keep track of how many bytes we read/sent so far.
			total_bytes = total_bytes + sent_bytes;

			/*printf("Client: sent to server %zi bytes. Total bytes sent so far = %i.\n", sent_bytes, total_bytes);*/

		}
		fclose(fptr);
		printf("%i bytes uploaded successfully.\n", total_bytes);
		close(client_socket);
		start_client("0", ip_address, 0);
	}
}

void client_download(int client_socket, char* command_line, char* filename)
{
	send(client_socket, command_line, MAXLINE, 0);        // send command to server
	if (!receive_ok(client_socket))
	{
		printf("File [%s] could not be found in remote directory.\n", filename);
	}
	else
	{
		int received_size;
		int total_bytes = 0;  // Keep track of how many bytes we downloaded so far.

		char destination_path[100];
		strcpy(destination_path, "./Local Directory/");
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
			total_bytes += received_size;
			send_ok('K', client_socket);

			// The server has closed the connection.
			// Note: the server will only close the connection when the application terminates.
			/*if (received_size == 0){
				fclose(fptr);
				break;
			}*/
			// Writing the received bytes into disk.
			fwrite(&file_chunk, sizeof(char), received_size, fptr);
			
			if (!receive_ok(client_socket))
			{
				fclose(fptr);
				break;
			}
		}
		printf("%i bytes downloaded successfully.\n", total_bytes);
	}
}

void client_delete(int client_socket, char* command_line, char* filename)
{
	send(client_socket, command_line, MAXLINE, 0);        // send command to server
	if (!receive_ok(client_socket))
	{
		printf("File [%s] could not be found in remote directory.\n", filename);
	}
	else
	{
		printf("File deleted successfully.\n");
	}
}

void execute(char* line, int client_socket, char* ip_address, int* append_mode)
{
	//printf("append_mode: %i\n", *append_mode);
	if (*append_mode)
	{
		char append_cmd[6];     // Destination string
		strncpy(append_cmd, line, 5);
		append_cmd[5] = 0; // null terminate destination

		if (strcmp(append_cmd, "pause") == 0)
		{
			char** args = tokenize(line);
			char* ptr;
        	int secs = strtol(args[1], &ptr, 10);
			sleep(secs);
		}
		else if (strcmp(append_cmd, "close") == 0)
		{
			printf("append mode: cmd close\n");
			// TODO: possibly close file, depending on implementation
			*append_mode = 0;
		}
		else
		{
			// TODO: append line to file on server
			printf("appending a line\n");
		}
	}
	else
	{
		char* command_line = (char *) malloc(strlen(line)+1);
		strncpy(command_line, line, strlen(line));
		command_line[strlen(line)+1] = 0;
		char** args = tokenize(line);
		if (strcmp(args[0], "pause") == 0)
		{
			char* ptr;
        	int secs = strtol(args[1], &ptr, 10);
			sleep(secs);
		}
		else if (strcmp(args[0], "append") == 0)
		{
			// TODO: probably open up file on server end
			printf("append cmd\n");
			*append_mode = 1;
		}
		else if (strcmp(args[0], "upload") == 0)
		{
			// TODO: upload local file to server
			client_upload(client_socket, ip_address, command_line, args[1]);
		}
		else if (strcmp(args[0], "download") == 0)
		{
			// TODO: download server file to local
			client_download(client_socket, command_line, args[1]);
		}
		else if (strcmp(args[0], "delete") == 0)
		{
			// TODO: delete server file
			client_delete(client_socket, command_line, args[1]);
		}
		else if (strcmp(args[0], "syncheck") == 0)
		{
			// TODO: something
			printf("syncheck cmd\n");
		}
		else if (strcmp(args[0], "quit") == 0)
		{
			free(command_line);
			exit(0);
		}
		else
		{
			// TODO: weird command
			printf("Command [%s] is not recognized.\n", command_line);
		}
		bzero(command_line, 100);
		free(command_line);
	}
}

int client_process(int client_socket, char* ip_address, char* user_commands)
{
	printf("Welcome to ICS53 Online Cloud Storage.\n");
	int append_mode = 0;
    char line[MAXLINE];

    /* Open file */
    FILE *file = fopen(user_commands, "r");
    
    if (!file)
    {
        perror(user_commands);
        return EXIT_FAILURE;
    }
    else
	{
		/* Get each line until there are none left */
		while (fgets(line, MAXLINE, file))
		{
			// Strip newline from command
			line[strcspn(line, "\n")] = '\0';

			/* Print each command/line 		TODO: when in append mode, prints Appending> */ 
			if (append_mode == 0)
			{
				printf("> %s\n", line);
			}
			else
			{
				printf("Appending> %s\n", line);
			}
			execute(line, client_socket, ip_address, &append_mode);

			//send(client_socket, line, strlen(line), 0);
			//sleep(1);
		}
		
		/* Close file */
		fclose(file);
	}

    close(client_socket);
    return 0;
}


int start_client(char* user_commands, char* ip_address, int start_process)
{
    int client_socket;
    struct sockaddr_in serv_addr;

    client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
    if (client_socket < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // The server IP address should be supplied as an argument when running the application.
    int addr_status = inet_pton(AF_INET, ip_address, &serv_addr.sin_addr);
    if (addr_status <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }
    int connect_status = connect(client_socket, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    if (connect_status < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }
	printf("client connected to server\n");

    ///////////// Start sending and receiving process //////////////
	if (start_process)
	{
		client_process(client_socket, ip_address, user_commands);
	}
    return 0;
}

int main(int argc, char *argv[])
{
	char* user_commands = argv[1];
	char* ip_address = argv[2];
	printf("I am the client.\n");
	printf("Input file name: %s\n", argv[1]);
	printf("My server IP address: %s\n", argv[2]);
	md5_print();
	printf("-----------\n");
	
	start_client(user_commands, ip_address, 1);

	return 0;
}
