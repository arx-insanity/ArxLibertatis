#ifndef ARX_NETWORK_CLIENT_DATA_H
#define ARX_NETWORK_CLIENT_DATA_H

class ClientData;

#include <string>
#include <thread>
#include "game/Entity.h"

#include "network/Server.h"

class ClientData {
  public:
    ClientData(int descriptor, Server * server);
    int getDescriptor();
    std::string getNickname();
    void write(std::string message);
    void listen();
    void stopListening();

  private:
    void connectionHandler();
    std::string read();

    Server * m_server;
    int m_descriptor;
    std::string m_nickname;
    Entity * m_entity;
    std::thread * m_thread;
};

#endif // ARX_NETWORK_CLIENT_DATA_H
