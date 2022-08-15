#include "network/server.h"

#define NUMBER_OF_CONNECTIONS 3

extern PlatformInstant REQUEST_JUMP;

int clients[NUMBER_OF_CONNECTIONS];
int numberOfClients = 0;
int serverSocketDescriptor;

void *startServer(void *) {
  int new_socket;
  int c;
  void *new_sock;
  struct sockaddr_in server;
  struct sockaddr_in client;
  char *message;
  
  // Create socket
  serverSocketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocketDescriptor == -1) {
    printf("Could not create socket");
  }
  
  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( 8888 );
  
  // Bind
  if (bind(serverSocketDescriptor,(struct sockaddr *)&server, sizeof(server)) < 0) {
    puts("bind failed");
    return NULL;
  }
  puts("bind done");
  
  // Listen
  listen(serverSocketDescriptor, NUMBER_OF_CONNECTIONS);
  
  // Accept and incoming connection
  puts("Waiting for incoming connections...");
  c = sizeof(struct sockaddr_in);
  while ((new_socket = accept(serverSocketDescriptor, (struct sockaddr *)&client, (socklen_t*)&c))) {
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

void stopServer() {
  puts("Stopping server");
  close(serverSocketDescriptor);
}

void __broadcast(int sender, std::string message) {
  char messageToBroadcast[3000];

  int size = sprintf(messageToBroadcast, "client #%d: %s\n", sender, message);

  if (message == "jump") {
    REQUEST_JUMP = g_platformTime.frameStart();
  }

  printf("%s", messageToBroadcast);

  for (char i = 0; i < numberOfClients; i++) {
    if (clients[i] != sender) {
      write(clients[i], messageToBroadcast, size);
    }
  }
}

void __connect(int clientId) {
  clients[numberOfClients] = clientId;
  numberOfClients += 1;

  char message[200];
  sprintf(message, "client #%d joined the server", clientId);
  __broadcast(clientId, message);
}

void __disconnect(int clientId) {
  printf("client #%d disconnected!\n", clientId);
  // TODO: remove sock from clients
  // TODO: broadcast the disconnection of client
}

void *connection_handler(void *clientSocketDescriptor) {
  // Get the socket descriptor
  int sock = *(int*)clientSocketDescriptor;
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
    raw_read_size = recv(sock, client_message, 2000, 0);

    /*
    // trimming newline chars
    read_size = raw_read_size;
    if (read_size > 0 && client_message[read_size - 2] == 13 && client_message[read_size - 1] == 10) {
      client_message[read_size - 2] = 0;
      read_size -= 2;
    }
    */

    std::string messageAsString(client_message, raw_read_size);
    boost::trim(messageAsString);
    read_size = messageAsString.length();

    if (read_size > 0) {
      __broadcast(sock, messageAsString);
    }
  } while (raw_read_size > 0);
  
  if (read_size == 0) {
    fflush(stdout);
  } else if (read_size == -1) {
    perror("recv failed");
  }

  // Free the socket pointer
  free(clientSocketDescriptor);
  __disconnect(sock);

  return 0;
}
