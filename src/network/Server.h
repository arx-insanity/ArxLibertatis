#ifndef ARX_NETWORK_SERVER_H
#define ARX_NETWORK_SERVER_H

#include <vector>
#include <thread>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>

#include "core/GameTime.h"
#include "gui/Notification.h"
*/

#include "game/Entity.h"
#include "io/log/Logger.h"
// #include "network/Client.h"

/*
void *startServer(void *portArg);

void stopServer();

void __connect(int clientId);

void __disconnect(int clientId);

void __broadcast(int sender, std::string message);
*/

void *connection_handler(void *clientSocketDescriptor);

struct clientInfo {
  int descriptor;
  std::string nickname;
  Entity * entity;
};

class Server {
  public:
    Server();
    void start(int port);
    void stop();

  private:
    clientInfo * findClientByDescriptor(int descriptor);
    void serverThread();

    int m_port;
    bool m_isRunning;
    int m_socketDescriptor;

    std::vector<clientInfo> m_clients;
};

#endif // ARX_NETWORK_SERVER_H
