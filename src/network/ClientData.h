#ifndef ARX_NETWORK_CLIENT_DATA_H
#define ARX_NETWORK_CLIENT_DATA_H

#include <thread>
#include "game/Entity.h"

class ClientData {
  public:
    ClientData(int descriptor);
    int getDescriptor();
    void write(std::string message);
    void listen();
    void stopListening();

  private:
    void connectionHandler();

    int m_descriptor;
    std::string m_nickname;
    Entity * m_entity;
};

#endif // ARX_NETWORK_CLIENT_DATA_H
