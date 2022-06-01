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

char receive_ok(client_socket)
{
	char okay[1];
	recv(client_socket, okay, 1, 0);		   // receive OK (or smt else) from server
	return okay[0];
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
		send(client_socket, command_line, MAXLINE, 0);        // send command to server
		if (receive_ok(client_socket) != 'K')
		{
			printf("File [%s] is currently locked by another user.\n", filename);
			return;
		}

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
	char ok_response = receive_ok(client_socket);
	if (ok_response == 'N')
	{
		printf("File [%s] could not be found in remote directory.\n", filename);
	}
	else if (ok_response == 'A')
	{
		printf("File [%s] is currently locked by another user.\n", filename);
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
			
			if (receive_ok(client_socket) == 'N')
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
	char ok_response = receive_ok(client_socket);
	if (ok_response == 'N')
	{
		printf("File [%s] could not be found in remote directory.\n", filename);
	}
	else if (ok_response == 'A')
	{
		printf("File [%s] is currently locked by another user.\n", filename);
	}
	else
	{
		printf("File deleted successfully.\n");
	}
}

void client_append(int client_socket, char* command_line, char* filename, int* append_mode)
{
	if (*append_mode)
	{
		// In append mode: Treat "commands" as strings to append, except pause and close
		char append_cmd[6];
		strncpy(append_cmd, command_line, 5);
		append_cmd[5] = 0;

		if (strcmp(append_cmd, "pause") == 0)
		{
			char** args = tokenize(command_line);
			char* ptr;
        	int secs = strtol(args[1], &ptr, 10);
			sleep(secs);
		}
		else if (strcmp(append_cmd, "close") == 0)
		{
			send(client_socket, command_line, MAXLINE, 0);
			*append_mode = 0;
		}
		else
		{
			send(client_socket, command_line, MAXLINE, 0);
		}	
	}
	else
	{
		// Not in append mode: Try and open file on server, if successful go into append mode
		send(client_socket, command_line, MAXLINE, 0);        // send command to server
		char ok_response = receive_ok(client_socket);
		if (ok_response == 'N')
		{
			printf("File [%s] could not be found in remote directory.\n", filename);
		}
		else if (ok_response == 'A')
		{
			printf("File [%s] is currently locked by another user.\n", filename);
		}
		else
		{
			*append_mode = 1;
		}
	}
}

void client_syncheck(int client_socket, char* command_line, char* filename)
{
	struct stat stats;
	send(client_socket, command_line, MAXLINE, 0);        // send command to server

	if (receive_ok(client_socket) != 'K') 	// File doesn't exist on remote directory.
	{	
		send_ok('K', client_socket);

		char source_path[100];
		strcpy(source_path, "./Local Directory/");
		strcat(source_path, filename);

		if (stat(source_path, &stats) != 0) 	// File doesn't exist on local either.
		{
			printf("File [%s] could not be found in local or remote directory.\n", filename);
		}
		else		// File exists only on local.
		{
			int filesize = stats.st_size;
			printf("Sync Check Report:\n");
			printf("- Local Directory:\n");
			printf("-- File Size: %i bytes.\n", filesize);
		}
	}
	else	// File exists on remote.
	{	
		send_ok('K', client_socket);

		char source_path[100];
		strcpy(source_path, "./Local Directory/");
		strcat(source_path, filename);

		char remote_filesize_buf[15]; 
		recv(client_socket, remote_filesize_buf, 15, 0);	// Receive file size of remote file
		send_ok('K', client_socket);

		// Receive md5 of file on the server
		char remote_md5_buf[17];
		recv(client_socket, remote_md5_buf, 17, 0);
		send_ok('K', client_socket);

		// TODO: receive lock status of the file from the server (mutex)
		char lock_status_buf[1];
		recv(client_socket, lock_status_buf, 1, 0);
		send_ok('K', client_socket);
		char* lock_status;
		if (lock_status_buf[0] == '1')
		{
			lock_status = "locked";
		}
		else
		{
			lock_status = "unlocked";
		}

		if (stat(source_path, &stats) != 0) 	// File ONLY exists on remote.
		{
			printf("Sync Check Report:\n");
			printf("- Remote Directory:\n");
			printf("-- File Size: %s bytes.\n", remote_filesize_buf);
			printf("-- Sync Status: unsynced.\n");
			printf("-- Lock Status: %s.\n", lock_status);
		}
		else		// File exists on BOTH remote and local.
		{
			// Get Md5 hash of local file, compare local and server md5 hashes
			char local_md5_buf[17];
			MDFile(source_path, local_md5_buf);
			local_md5_buf[16] = 0; 

			char* sync_status;
			if (strcmp(local_md5_buf, remote_md5_buf) == 0)
			{
				sync_status = "synced";
			}
			else
			{
				sync_status = "unsynced";
			}

			printf("Sync Check Report:\n");
			printf("- Local Directory:\n");
			printf("-- File Size: %i bytes.\n", (int)stats.st_size);
			printf("- Remote Directory:\n");
			printf("-- File Size: %s bytes.\n", remote_filesize_buf);
			printf("-- Sync Status: %s.\n", sync_status);
			printf("-- Lock Status: %s.\n", lock_status);
		}
	}
}

void execute(char* line, int client_socket, char* ip_address, int* append_mode)
{
	//printf("append_mode: %i\n", *append_mode);
	if (*append_mode)
	{
		client_append(client_socket, line, "0", append_mode);
	}
	else
	{
		char* command_line = (char *) malloc(MAXLINE);
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
			client_append(client_socket, command_line, args[1], append_mode);
		}
		else if (strcmp(args[0], "upload") == 0)
		{
			client_upload(client_socket, ip_address, command_line, args[1]);
		}
		else if (strcmp(args[0], "download") == 0)
		{
			client_download(client_socket, command_line, args[1]);
		}
		else if (strcmp(args[0], "delete") == 0)
		{
			client_delete(client_socket, command_line, args[1]);
		}
		else if (strcmp(args[0], "syncheck") == 0)
		{
			client_syncheck(client_socket, command_line, args[1]);
		}
		else if (strcmp(args[0], "quit") == 0)
		{
			free(command_line);
			exit(0);
		}
		else
		{
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

			/* Print each command/line 		TODO: don't print input command on line */ 
			if (append_mode == 0)
			{
				//printf("> %s\n", line);
				printf("> \n");
			}
			else
			{
				//printf("Appending> %s\n", line);
				printf("Appending> \n");
			}
			execute(line, client_socket, ip_address, &append_mode);
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
	//printf("client connected to server\n");

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
	/*printf("I am the client.\n");
	printf("Input file name: %s\n", argv[1]);
	printf("My server IP address: %s\n", argv[2]);
	printf("-----------\n");*/

	start_client(user_commands, ip_address, 1);

	return 0;
}
