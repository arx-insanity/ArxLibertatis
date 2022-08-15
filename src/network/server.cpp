#include<stdio.h>
#include<string.h> // strlen
#include<stdlib.h> // strlen
#include<sys/socket.h>
#include<arpa/inet.h> // inet_addr, htons
#include<unistd.h> // write

#include<pthread.h> // for threading, link with lpthread

#include "core/GameTime.h"

#define NUMBER_OF_CONNECTIONS 3

void *connection_handler(void *);

int clients[NUMBER_OF_CONNECTIONS];
int numberOfClients = 0;

void *startServer(void *tmp);

void *startServer(void *tmp) {
  int socket_desc;
  int new_socket;
  int c;
  void *new_sock;
  struct sockaddr_in server;
  struct sockaddr_in client;
  char *message;
  
  // Create socket
  socket_desc = socket(AF_INET, SOCK_STREAM, 0);
  if (socket_desc == -1) {
    printf("Could not create socket");
  }
  
  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( 8888 );
  
  // Bind
  if (bind(socket_desc,(struct sockaddr *)&server, sizeof(server)) < 0) {
    puts("bind failed");
    return NULL;
  }
  puts("bind done");
  
  // Listen
  listen(socket_desc, NUMBER_OF_CONNECTIONS);
  
  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  while ((new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c))) {
    puts("Connection accepted");
    
    // Reply to the client
    message = "Hello Client, I have received your connection. And now I will assign a handler for you\n";
    write(new_socket, message, strlen(message));
    
    pthread_t sniffer_thread;
    new_sock = malloc(1);
    *(int*)new_sock = new_socket;
    
    if (pthread_create( &sniffer_thread, NULL,  connection_handler, new_sock) < 0) {
      perror("could not create thread");
      return NULL;
    }
    
    // Now join the thread, so that we dont terminate before the thread
    // pthread_join( sniffer_thread, NULL);
    puts("Handler assigned");
  }
  
  if (new_socket < 0) {
    perror("accept failed");
    return NULL;
  }
  
  return NULL;
}

extern PlatformInstant REQUEST_JUMP;

void __connect(int clientId);

void __connect(int clientId) {
  clients[numberOfClients] = clientId;
  numberOfClients += 1;
  printf("client #%d connected!\n", clientId);
  // TODO: broadcast the connection of new client
}

void __disconnect(int clientId);

void __disconnect(int clientId) {
  printf("client #%d disconnected!\n", clientId);
  // TODO: remove sock from clients
  // TODO: broadcast the disconnection of client
}

void __broadcast(int sender, char *message);

void __broadcast(int sender, char *message) {
  char messageToBroadcast[3000];

  int size = sprintf(messageToBroadcast, "client #%d: %s\n", sender, message);

  if (memcmp(message, "jump", 4) == 0) {
    REQUEST_JUMP = g_platformTime.frameStart();
  }

  printf("%s", messageToBroadcast);

  for (char i = 0; i < numberOfClients; i++) {
    if (clients[i] != sender) {
      write(clients[i], messageToBroadcast, size);
    }
  }
}

void *connection_handler(void *socket_desc);

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc) {
  // Get the socket descriptor
  int sock = *(int*)socket_desc;
  int read_size;
  char *message;
  char client_message[2000];

  __connect(sock);
  
  // Send some messages to the client
  message = "Greetings! I am your connection handler\n";
  write(sock, message, strlen(message));
  
  message = "Send some text to the others!\n";
  write(sock, message, strlen(message));

  int raw_read_size = 0;

  // Receive a message from client
  do {
    read_size = recv(sock, client_message, 2000, 0);
    raw_read_size = read_size;


    // trimming newline chars
    if (read_size > 0 && client_message[read_size - 2] == 13 && client_message[read_size - 1] == 10) {
      client_message[read_size - 2] = 0;
      read_size -= 2;
    }

    if (read_size > 0) {
      __broadcast(sock, client_message);
    }
  } while (raw_read_size > 0);
  
  if (read_size == 0) {
    fflush(stdout);
  } else if (read_size == -1) {
    perror("recv failed");
  }

  // Free the socket pointer
  free(socket_desc);
  __disconnect(sock);

  return 0;
}
