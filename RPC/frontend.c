#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "a1_lib.h"

#define BUFSIZE 1024

// struct to resemble packet/message to be sent
struct PACKET{
    char *command;
    char *parameters[2];
}packet;

void parse_input(char *input);
void toJSON();

char message[BUFSIZE] = {0};

int main(int argc, char *argv[]) {
  // incorrect usgae
  if (argc != 3) {
    fprintf(stderr, "usage: ./frontend <host_ip> <host_port>\n");
    return -2;
  }

  char *ip = argv[1];
  int port = atoi(argv[2]);

  int sockfd;
  char user_input[BUFSIZE] = {0};
  char server_msg[BUFSIZE] = {0};

  // try to initiate connection to socket
  if (connect_to_server(ip, port, &sockfd) < 0) {
    fprintf(stderr, "oh no\n");
    return -1;
  }
  while (strcmp(user_input, "exit\n")) {
    printf(">> ");

    memset(user_input, 0, sizeof(user_input));
    memset(server_msg, 0, sizeof(server_msg));
    memset(message, 0, sizeof(message));

    // read user input from command line
    fgets(user_input, BUFSIZE, stdin);
    
    // parse user input
    parse_input(user_input);

    // put user input in JSON format
    toJSON();

    // send the parsed input to server
    send_message(sockfd, message, sizeof(message));

    // reset packet contents
    memset(packet.command, 0, sizeof(packet.command));
    if (packet.parameters[0])
      memset(packet.parameters[0], 0, sizeof(packet.parameters[0]));
    if (packet.parameters[1])
      memset(packet.parameters[1], 0, sizeof(packet.parameters[1]));

    //receive a msg from the server
    ssize_t byte_count = recv_message(sockfd, server_msg, sizeof(server_msg));
    if (byte_count <= 0) {
      break;
    }
    printf("%s\n", server_msg);
  }

  return 0;
}

/*
 * take user input and split it up to be stored in packet struct
 */
void parse_input(char *input) {
    char *token = strtok(input, " ");
    packet.command = strdup(token);
    token = strtok(NULL, " ");
    if(token) {
      packet.parameters[0] = strdup(token);
    }
    token = strtok(NULL, " ");
    if(token) {
      packet.parameters[1] = strdup(token);
    }
}

/*
 * split command and parameters into JSON format
 * example: command: "add", parameters: "1 2"
 */
void toJSON() {
  strcat(message, "command: \"");
  strcat(message, packet.command);
  strcat(message, "\", ");

  strcat(message, "parameters: \"");
  if (packet.parameters[0]) {
    strcat(message, packet.parameters[0]);

    if (packet.parameters[1]) {
      strcat(message, " ");
      strcat(message, packet.parameters[1]);
    }
  }
  strcat(message, "\"");
}
