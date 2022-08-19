#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/Server.h"

// TODO: make this a member of the Server class
std::thread m_thread;

Server::Server() {
  this->m_isRunning = false;
}

void Server::start(int port) {
  if (this->m_isRunning) {
    LogError << SERVER_PREFIX << "server already started";
    return;
  }

  LogInfo << SERVER_PREFIX << "server starting...";

  this->m_port = port;

  std::function<void(void)> fn = std::bind(&Server::connectionHandler, this);
  m_thread = std::thread(fn);
}

void Server::stop() {
  if (!this->m_isRunning) {
    LogError << SERVER_PREFIX << "server not running";
    return;
  }

  LogInfo << SERVER_PREFIX << "stopping server...";

  this->m_isRunning = false;
  shutdown(this->m_socketDescriptor, SHUT_RD);
  close(this->m_socketDescriptor);

  if (!this->m_clients.empty()) {
    for(unsigned long int i = 0; i < this->m_clients.size(); i++) {
      this->m_clients[i]->stopListening();
    }
  }

  m_thread.join();

  LogInfo << SERVER_PREFIX << "server stopped";
}

ClientData * Server::findClientByDescriptor(int descriptor) {
  if (this->m_clients.empty()) {
    return nullptr;
  }

  for (long unsigned int i = 0; i < this->m_clients.size(); i++) {
    if (this->m_clients[i]->getDescriptor() == descriptor) {
      return this->m_clients[i];
    }
  }

  return nullptr;
}

void Server::connectionHandler() {
  this->m_isRunning = true;

  this->m_socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (this->m_socketDescriptor == -1) {
    LogError << SERVER_PREFIX << "could not create socket";
    return;
  }

  sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(this->m_port);

  int resultOfBinding = bind(this->m_socketDescriptor, (sockaddr *)&server, sizeof(server));
  if (resultOfBinding < 0) {
    LogError << SERVER_PREFIX << "bind failed";
    return;
  }

  listen(this->m_socketDescriptor, 3);

  LogInfo << SERVER_PREFIX << "server started at port " << std::to_string(this->m_port);

  socklen_t c = sizeof(sockaddr_in);
  sockaddr_in client;
  int clientDescriptor;

  while ((clientDescriptor = accept(this->m_socketDescriptor, (sockaddr *)&client, &c)) > 0) {
    LogInfo << SERVER_PREFIX << "client connecting...";

    ClientData clientData(clientDescriptor);
    clientData.write(SERVER_PREFIX + "connecting...");
    clientData.listen();

    this->m_clients.push_back(&clientData);
  }

  if (clientDescriptor < 0 && this->m_isRunning) {
    LogError << SERVER_PREFIX << "accepting connections failed";
    return;
  }
}

/*
extern PlatformInstant REQUEST_JUMP;

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

void __broadcast(int sender, std::string message) {
  char messageToBroadcast[3000];

  int size = sprintf(messageToBroadcast, "client #%d: %s\n", sender, message.c_str());

  // if (message == "jump") {
  //   // make the player jump
  //   REQUEST_JUMP = g_platformTime.frameStart();
  // }

  clientData * client = findClientById(sender);
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
  clientData client;
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
