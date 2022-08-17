#include "network/Server.h"

#define LOGPREFIX "Arx Server: "

// TODO: make this a member of the Server class
std::thread m_thread;

Server::Server() {
  this->m_isRunning = false;
}

void Server::start(int port) {
  if (this->m_isRunning) {
    LogError << LOGPREFIX << "server already started";
    return;
  }

  LogInfo << LOGPREFIX << "server starting...";

  this->m_port = port;

  std::function<void(void)> fn = std::bind(&Server::serverThread, this);

  m_thread = std::thread(fn);
}

void Server::stop() {
  if (!this->m_isRunning) {
    LogError << LOGPREFIX << "server not running";
    return;
  }

  LogInfo << LOGPREFIX << "stopping server...";

  this->m_isRunning = false;
  shutdown(this->m_socketDescriptor, SHUT_RD);
  close(this->m_socketDescriptor);
  m_thread.join();

  LogInfo << LOGPREFIX << "server stopped";
}

clientInfo * Server::findClientByDescriptor(int descriptor) {
  if (this->m_clients.empty()) {
    return nullptr;
  }

  for (long unsigned int i = 0; i < this->m_clients.size(); i++) {
    if (this->m_clients[i].descriptor == descriptor) {
      return &this->m_clients[i];
    }
  }

  return nullptr;
}

void Server::serverThread() {
  this->m_isRunning = true;

  this->m_socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (this->m_socketDescriptor == -1) {
    LogError << LOGPREFIX << "could not create socket";
    return;
  }

  struct sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(this->m_port);

  int resultOfBinding = bind(this->m_socketDescriptor, (struct sockaddr *)&server, sizeof(server));
  if (resultOfBinding < 0) {
    LogError << LOGPREFIX << "bind failed";
    return;
  }

  listen(this->m_socketDescriptor, 3);

  LogInfo << LOGPREFIX << "server started at port " << std::to_string(this->m_port);

  socklen_t c = sizeof(struct sockaddr_in);

  struct sockaddr_in client;
  int clientDescriptor;

  while ((clientDescriptor = accept(this->m_socketDescriptor, (struct sockaddr *)&client, &c)) > 0) {
    LogInfo << LOGPREFIX << "connection accepted";

    // TODO: create thread for client

    // pthread_t sniffer_thread;
    // void *new_sock;
    // new_sock = malloc(1);
    // *(int*)new_sock = new_socket;
    
    // if (pthread_create( &sniffer_thread, NULL,  connection_handler, new_sock) < 0) {
    //   LogError << LOGPREFIX << "Could not create thread for the connection handler";
    //   return;
    // }
  }

  if (clientDescriptor < 0 && this->m_isRunning) {
    LogError << LOGPREFIX << "accepting connections failed";
    return;
  }

  /*
    message = "Arx Server: Connected\n";
    write(new_socket, message, strlen(message));

    message = "Arx Server: Assigning handler...\n";
    write(new_socket, message, strlen(message));

    LogInfo << LOGPREFIX << "Connection handler assigned to client";
  }
  */
}

/*
extern PlatformInstant REQUEST_JUMP;

std::vector<clientData> clients;

int findClientIndexById(const std::vector<clientData> &clients, int clientId) {
  if (clients.empty()) {
    return -1;
  }

  for (int i = 0; i < clients.size(); i++) {
    if (clients[i].clientId == clientId) {
      return i;
    }
  }

  return -1;
}

void *startServer(void *portArg) {
  unsigned int port = *(unsigned int*)portArg;
  int new_socket;
  int c;
  void *new_sock;
  struct sockaddr_in server;
  struct sockaddr_in client;
  char *message;

  self.clientId = 'server';
  self.nickname = 'Server';
  
  // Create socket
  serverSocketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (serverSocketDescriptor == -1) {
    LogError << LOGPREFIX << "could not create socket";
    return nullptr;
  }
  
  // Prepare the sockaddr_in structure
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(port);

  LogInfo << LOGPREFIX << "binding done, created server at port " << port;

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

  // if (message == "jump") {
  //   // make the player jump
  //   REQUEST_JUMP = g_platformTime.frameStart();
  // }

  struct clientData * client = findClientById(sender);
  // TODO: in the future sender might be the server, compare it with self

  if (boost::starts_with(message, "/")) {
    std::string::size_type commandSize = message.find(" ", 0);
    std::string command = message.substr(1, commandSize - 1);
    std::string text = boost::trim_copy(boost::erase_head_copy(message, commandSize));

    if (command == "say") {
      if (!text.empty()) {
        notification_add(std::string((*client).nickname + ": " + text));
      }
    } else if (command == "nickname") {
      if (!text.empty()) {
        (*client).nickname = text;
      }
    }
  }

  LogInfo << LOGPREFIX << "client #" << sender << ": " << message;

  for (char i = 0; i < clients.size(); i++) {
    if (clients[i].clientId != sender) {
      write(clients[i].clientId, messageToBroadcast, size);
    }
  }
}

void __connect(int clientId) {
  struct clientData client;
  client.clientId = clientId;
  client.nickname = "client #" + std::to_string(clientId);

  clients.push_back(client);

  __broadcast(clientId, "joined the server");
}

void __disconnect(int clientId) {
  int idx = findClientIndexById(clients, clientId);
  if (idx != -1) {
    __broadcast(clientId, "disconnected from the server");
    clients.erase(clients.begin() + idx);
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

    if (messageAsString == "/exit" || messageAsString == "/quit") {
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
*/
