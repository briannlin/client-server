// The 'client.c' code goes here.
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "Md5.c"  // Feel free to include any other .c files that you need in the 'Client Domain'.
#define PORT 9999
#define MAXLINE 500
#define MAXARGS 3

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

void execute(char* line, int client_socket, int* append_mode)
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
			printf("upload cmd\n");
		}
		else if (strcmp(args[0], "download") == 0)
		{
			// TODO: download server file to local
			printf("download cmd\n");
		}
		else if (strcmp(args[0], "delete") == 0)
		{
			// TODO: delete server file
			printf("delete cmd\n");
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
		free(command_line);
	}
}

int client_example_loop(int client_socket, char* user_commands)
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
			execute(line, client_socket, &append_mode);

			//send(client_socket, line, strlen(line), 0);
			//sleep(1);
		}
		
		/* Close file */
		fclose(file);
	}

    close(client_socket);
    return 0;
}


int start_client(char* user_commands, char* ip_address)
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
    client_example_loop(client_socket, user_commands);
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
	
	start_client(user_commands, ip_address);

	return 0;
}
