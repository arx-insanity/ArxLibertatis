#include <sys/socket.h>
#include <arpa/inet.h>
#include "io/log/Logger.h"
#include "core/GameTime.h"
#include "gui/Notification.h"
#include "network/common.h"
#include "network/Server.h"

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

  this->m_thread = new std::thread(&Server::connectionHandler, this);
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

  this->m_thread->join();

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

    ClientData * clientData = new ClientData(clientDescriptor, this);
    clientData->write(SERVER_PREFIX + "connecting...");
    clientData->listen();

    this->m_clients.push_back(clientData);
  }

  if (clientDescriptor < 0 && this->m_isRunning) {
    LogError << SERVER_PREFIX << "accepting connections failed";
    return;
  }
}

void Server::disconnect(ClientData * client) {
  if (this->m_clients.empty()) {
    return;
  }

  for (long unsigned int i = 0; i < this->m_clients.size(); i++) {
    if (this->m_clients[i] == client) {
      this->broadcast(client, "disconnected");
      this->m_clients.erase(this->m_clients.begin() + i);
      return;
    }
  }
}

void Server::broadcast(ClientData * client, std::string event, std::string args) {
  if (event == "joined") {
    for (long unsigned int i = 0; i < this->m_clients.size(); i++) {
      if (this->m_clients[i] != client) {
        this->m_clients[i]->write(client->getNickname() + " joined the server");
      }
    }
  } else if (event == "disconnected") {
    for (long unsigned int i = 0; i < this->m_clients.size(); i++) {
      if (this->m_clients[i] != client) {
        this->m_clients[i]->write(client->getNickname() + " disconnected");
      }
    }
  } else if (event == "say") {
    for (long unsigned int i = 0; i < this->m_clients.size(); i++) {
      this->m_clients[i]->write(client->getNickname() + ": " + args);
    }
  } else if (event == "make-host") {
    if (args == "jump") {
      extern PlatformInstant REQUEST_JUMP;
      REQUEST_JUMP = g_platformTime.frameStart();
    } else {
      client->write("unknown argument '" + args + "' for /make-host");
    }
  }
}
