#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "io/log/Logger.h"
#include "core/GameTime.h"
#include "gui/Notification.h"
#include "network/common.h"
#include "network/Server.h"

Server::Server(int port) {
  this->m_port = port;
  this->m_isRunning = false;
}

void Server::start() {
  if (this->m_isRunning) {
    LogError << "Can't start server, already started";
    return;
  }

  LogInfo << "Server starting...";

  this->m_thread = new std::thread(&Server::connectionHandler, this);
}

void Server::stop() {
  if (!this->m_isRunning) {
    LogError << "Can't stop server, not running";
    return;
  }

  LogInfo << "Stopping server...";

  this->m_isRunning = false;
  shutdown(this->m_socketDescriptor, SHUT_RDWR);
  close(this->m_socketDescriptor);

  if (!this->m_clients.empty()) {
    for(unsigned long int i = 0; i < this->m_clients.size(); i++) {
      this->m_clients[i]->write(MessageTypeServerStopped);
      this->m_clients[i]->stopListening();
    }
  }

  this->m_thread->join();

  LogInfo << "Server stopped";
}

ClientData * Server::findClientByDescriptor(int descriptor) {
  if (this->m_clients.empty()) {
    return nullptr;
  }

  for (unsigned long int i = 0; i < this->m_clients.size(); i++) {
    if (this->m_clients[i]->getDescriptor() == descriptor) {
      return this->m_clients[i];
    }
  }

  return nullptr;
}

void Server::connectionHandler() {
  this->m_isRunning = true;

  this->m_socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (this->m_socketDescriptor < 0) {
    LogError << "Could not create socket";
    return;
  }

  sockaddr_in server;
  server.sin_family = AF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons(this->m_port);

  int resultOfBinding = bind(this->m_socketDescriptor, (sockaddr *)&server, sizeof(server));
  if (resultOfBinding < 0) {
    LogError << "Bind failed";
    return;
  }

  listen(this->m_socketDescriptor, 3); // 3 = maximum connections?

  LogInfo << "Server started @ localhost:" << std::to_string(this->m_port);

  socklen_t c = sizeof(sockaddr_in);
  sockaddr_in client;
  int clientDescriptor;

  while ((clientDescriptor = accept(this->m_socketDescriptor, (sockaddr *)&client, &c)) > 0) {
    LogInfo << "Client connecting...";

    ClientData * clientData = new ClientData(clientDescriptor, this);
    clientData->listen();

    this->m_clients.push_back(clientData);
  }

  if (clientDescriptor < 0 && this->m_isRunning) {
    LogError << "Accepting connections failed";
    return;
  }
}

void Server::disconnect(ClientData * client) {
  if (this->m_clients.size() <= 1) {
    return;
  }

  for (unsigned long int i = 0; i < this->m_clients.size(); i++) {
    if (this->m_clients[i] == client) {
      this->m_clients.erase(this->m_clients.begin() + i);
      return;
    }
  }
}

void Server::broadcast(ClientData * client, MessageType messageType, std::string payload) {
  if (this->m_clients.empty()) {
    return;
  }

  for (unsigned long int i = 0; i < this->m_clients.size(); i++) {
    if (this->m_clients[i] != client) {
      this->m_clients[i]->write(messageType, payload);
    }
  }
}

bool Server::isRunning() {
  return this->m_isRunning;
}
