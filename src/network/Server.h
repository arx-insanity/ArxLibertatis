#ifndef ARX_NETWORK_SERVER_H
#define ARX_NETWORK_SERVER_H

class Server;

#include <thread>
#include <vector>
#include "network/ClientData.h"

class Server {
  public:
    Server();
    void start(int port);
    void stop();
    void disconnect(ClientData * client);
    void broadcast(ClientData * client, std::string event, std::string args = "");

  private:
    ClientData * findClientByDescriptor(int descriptor);
    void connectionHandler();

    int m_port;
    bool m_isRunning;
    int m_socketDescriptor;

    std::vector<ClientData *> m_clients;
    std::thread * m_thread;
};

#endif // ARX_NETWORK_SERVER_H
