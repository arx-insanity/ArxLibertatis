#include "network/server.h"

#define NUMBER_OF_CONNECTIONS 3
#define LOGPREFIX "Network: "

extern PlatformInstant REQUEST_JUMP;

std::vector<int> clients;

int serverSocketDescriptor;

int findIndex(const std::vector<int> &arr, int item) {
  auto ret = std::find(arr.begin(), arr.end(), item);

  if (ret != arr.end()) {
    return ret - arr.begin();
  }

  return -1;
}

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
    LogError << LOGPREFIX << "could not create socket";
    return nullptr;
  }
  
  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(8888);
  
  int resultOfBinding = bind(serverSocketDescriptor, (struct sockaddr *)&server, sizeof(server));
  if (resultOfBinding < 0) {
    LogError << LOGPREFIX << "bind failed";
    return nullptr;
  }

  LogInfo << LOGPREFIX << "binding done";

  listen(serverSocketDescriptor, NUMBER_OF_CONNECTIONS);

  // Accept and incoming connection
  LogInfo << LOGPREFIX << "Waiting for incoming connections...";
  c = sizeof(struct sockaddr_in);
  while ((new_socket = accept(serverSocketDescriptor, (struct sockaddr *)&client, (socklen_t*)&c))) {
    LogInfo << LOGPREFIX << "Connection accepted";
    
    message = "Arx Server: Connected\n";
    write(new_socket, message, strlen(message));

    message = "Arx Server: Assigning handler...\n";
    write(new_socket, message, strlen(message));
    
    pthread_t sniffer_thread;
    new_sock = malloc(1);
    *(int*)new_sock = new_socket;
    
    if (pthread_create( &sniffer_thread, NULL,  connection_handler, new_sock) < 0) {
      LogError << LOGPREFIX << "Could not create thread for the connection handler";
      return nullptr;
    }

    LogInfo << LOGPREFIX << "Connection handler assigned to client";
  }
  
  if (new_socket < 0) {
    LogError << LOGPREFIX << "Accepting connections failed";
    return nullptr;
  }

  return nullptr;
}

void stopServer() {
  LogInfo << LOGPREFIX << "Stopping server";

  close(serverSocketDescriptor);
}

void __broadcast(int sender, std::string message) {
  char messageToBroadcast[3000];

  int size = sprintf(messageToBroadcast, "client #%d: %s\n", sender, message.c_str());

  if (message == "jump") {
    REQUEST_JUMP = g_platformTime.frameStart();
  }

  LogInfo << LOGPREFIX << "client #" << sender << ": " << message;

  for (char i = 0; i < clients.size(); i++) {
    if (clients[i] != sender) {
      write(clients[i], messageToBroadcast, size);
    }
  }
}

void __connect(int clientId) {
  clients.push_back(clientId);

  __broadcast(clientId, "joined the server");
}

void __disconnect(int clientId) {
  int idx = findIndex(clients, clientId);
  if (idx != -1) {
    clients.erase(clients.begin() + idx);
    __broadcast(clientId, "disconnected from the server");
  }
}

void *connection_handler(void *clientSocketDescriptor) {
  int clientId = *(int*)clientSocketDescriptor;
  char *message;
  char client_message[2000];

  __connect(clientId);

  message = "Arx Server: Handler assigned, welcome to Arx!\n";
  write(clientId, message, strlen(message));

  int raw_read_size = 0;
  bool clientWantsToQuit = false;

  do {
    // Receive a message from client
    raw_read_size = recv(clientId, client_message, 2000, 0);

    std::string messageAsString(client_message, raw_read_size);
    boost::trim(messageAsString);

    if (messageAsString == "exit" || messageAsString == "quit") {
      std::string goodbyeMessage = "Arx Server: Disconnected, goodbye!\n";
      write(clientId, goodbyeMessage.c_str(), goodbyeMessage.size());
      clientWantsToQuit = true;
    }

    if (!messageAsString.empty() && !clientWantsToQuit) {
      __broadcast(clientId, messageAsString);
    }
  } while (raw_read_size > 0 && !clientWantsToQuit);
  
  if (raw_read_size == 0 || clientWantsToQuit) {
    // TODO: can we tell TELNET to quit?
    fflush(stdout);
  } else if (raw_read_size == -1) {
    LogError << LOGPREFIX << "Reading message from client using recv failed";
  }

  // Free the socket pointer
  free(clientSocketDescriptor);
  __disconnect(clientId);

  return 0;
}
