#ifndef ARX_NETWORK_SERVER_H
#define ARX_NETWORK_SERVER_H

class Server;

#include <thread>
#include <vector>
#include "network/ClientData.h"

class Server {
  public:
    Server(int port);
    void start();
    void stop();
    void disconnect(ClientData * client);
    void broadcast(ClientData * client, MessageType messageType, std::string payload = "");

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
