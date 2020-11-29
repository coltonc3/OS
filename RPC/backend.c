#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "a1_lib.h"

#define BUFSIZE 1024

// declare functions
void RPC_Register(char *command, char *function);
int addInts(int a, int b);
int multiplyInts(int a, int b);
uint64_t factorial(int x);
float divideFloats(float a, float b);
struct PACKET fromJSON(char *buffer);
int handleClient(char *msg, int clientfd);
int registered(char *cmd);

// struct to represent register for available commands and corresponding functions
struct FUNC_ENTRY {
	char *command;
	char *function;
}registry[50];

// struct to store message received from client
struct PACKET{
    char *command;
    char *parameters[2];
};

char result[50];
int running = 1;
int max_clients = 5;

int main(int argc, char *argv[]) {

	if (argc != 3) {
    fprintf(stderr, "usage: ./backend <host_ip> <host_port>\n");
    return -2;
  }

  char *ip = argv[1];
  int port = atoi(argv[2]);

	int sockfd, clientfd;
	char msg[BUFSIZE];

	if (create_server(ip, port, &sockfd) < 0)
	// if( create_server("127.0.0.1", 10003, &sockfd) < 0)
	{
			fprintf(stderr, "oh no\n");
			return -1;
	}

	// register command,function pairs into registry (lookup table)
	RPC_Register("add", "addInts");
	RPC_Register("multiply", "multiplyInts");
	RPC_Register("divide", "divideFloats");
	RPC_Register("factorial", "factorial");
	RPC_Register("exit\n", "exit");
	RPC_Register("sleep", "sleep");
	RPC_Register("shutdown\n", "shutdown");

	int num_connections = 0;

	while(running) {
		// if we have less than 5 clients connected then accept new connections
		if (num_connections < max_clients) {
			
			// start listening for new connection
			if (accept_connection(sockfd, &clientfd) < 0) {
				fprintf(stderr, "oh no\n");
				return -1;
			}
			num_connections++;
			printf("cnxns: %s\n", num_connections);
			// create child process to handle client
			int pid = fork();

			if (pid < 0) {
				printf("Fork failed!\n");
				return 1;
			}
			else if (pid == 0) {	// child
				int status = handleClient(msg, clientfd);  /* stores shutdown response */
				num_connections--;
				printf("%s\n", num_connections);

				// shutdown signal received 
				if (status) {
					printf("Server shutting down.\n");

					// kill parent process
					kill(getppid(), SIGINT);
					break;
				}
				exit(0);
			}
			else { //parent
				close(clientfd);				
			}
		}
	}
	return 0;
}

// handles a single client connection
int handleClient(char *msg, int clientfd) {
	struct PACKET message = {0, 0};
	message.command = "";

	// this will become 1 if shutdown is initiated
	int status = 0;

	
	while(strcmp(message.command, "exit\n")) {
		ssize_t byte_count = recv_message(clientfd, msg, BUFSIZE);
		if (byte_count <= 0) {
			break;
		}
		message = fromJSON(msg);
		
		int param_count = 0;
		if(registered(message.command) == 0){
			sprintf(result, "Error: Command \"%s\" not found", message.command);
		}
		else {
			// count number of parameters
			if (message.parameters[0]) {
				param_count++;
				if (message.parameters[1]) {
					param_count++;
				}
			}

			if (!strcmp(message.command, "add")) {
				if (param_count != 2) {
					sprintf(result, "%s", "need two numbers for add");
				}
				else {
					int num1 = atoi(message.parameters[0]);
					int num2 = atoi(message.parameters[1]);
					sprintf(result, "%d", addInts(num1, num2));
				}
			}
			else if (!strcmp(message.command, "multiply")) {
				if(param_count != 2) {
					sprintf(result, "%s", "need two numbers for multiply");						
				}
				else {
					int num1 = atoi(message.parameters[0]);
					int num2 = atoi(message.parameters[1]);
					sprintf(result, "%d", multiplyInts(num1, num2));
				}
			}
			else if (!strcmp(message.command, "divide")) {
				if(param_count != 2) {
					sprintf(result, "%s", "need two numbers for divide");
				}
				else if (atof(message.parameters[1]) == 0.0) {
					sprintf(result, "Error: division by zero");
				}
				else {
					float num1 = atof(message.parameters[0]);
					float num2 = atof(message.parameters[1]);
					sprintf(result, "%f", divideFloats(num1, num2));
				}
			}
			else if (!strcmp(message.command, "sleep")) {
				int num = atoi(message.parameters[0]);
				int res = sleep(num);
				sprintf(result, "%d", res);
			}
			else if (!strcmp(message.command, "factorial")) {
				int num1 = atoi(message.parameters[0]);
				sprintf(result, "%ld", factorial(num1));
			}
			else if (!strcmp(message.command, "exit\n")) {
				sprintf(result, "Disconnected");
				break;
			}
			else if (!strcmp(message.command, "shutdown\n")) {
				status = 1;
				break;
			}
		}		

		fflush(stdout);

		// send result back to client
		send_message(clientfd, result, strlen(result));

		memset(result, 0, sizeof(result));

		// reset message contents
		memset(message.command, 0, sizeof(message.command));
		memset(message.parameters[0], 0, sizeof(message.parameters[0]));
		memset(message.parameters[1], 0, sizeof(message.parameters[1]));
	}
	return status;
}

// returns 1 if command is in registry, 0 if not
int registered(char *cmd) {
	for (int i = 0; registry[i].command != NULL; i++) {
		// printf("comparing %s and %s\n",cmd, registry[i].command);
		if (strcmp(registry[i].command, cmd) == 0)
			return 1;
	}
	return 0;
}

// returns the sum of two integers
int addInts(int a, int b) {
	return a + b;
}

// returns the product of two integers
int multiplyInts(int a, int b) {
	return a * b;
}

// returns the quotient of two floats
float divideFloats(float a, float b) {
	return a / b;
}

// returns the factorial of an integer
uint64_t factorial(int x) {
	if (x == 1) {
		return 1;
	}
	return x * factorial(x - 1);
}

// takes JSON message from client and parses it into packet struct
struct PACKET fromJSON(char *buffer) {
	struct PACKET message;

	char *token = strtok(buffer, "\"");
	token = strtok(NULL, "\"");
	
	message.command = strdup(token);
	token = strtok(NULL, "\"");

	token = strtok(NULL, " ");
	if (token) {
		message.parameters[0] = strdup(token);
		token = strtok(NULL, "\"");
		if(token) {
			message.parameters[1] = strdup(token);
		}
	}
	return message;
}

// register available commands/functions in register
void RPC_Register(char *command, char *function) {
	for (int i = 0; i < sizeof(registry)/sizeof(registry[0]); i++) {
		if (registry[i].command == NULL) {
			registry[i].command = strdup(command);
			registry[i].function = strdup(function);
			break;
		}
	}
}
