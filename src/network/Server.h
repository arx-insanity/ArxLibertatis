#ifndef ARX_NETWORK_SERVER_H
#define ARX_NETWORK_SERVER_H

#include <thread>
#include <vector>
#include "network/ClientData.h"

/*
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>

#include "core/GameTime.h"
#include "gui/Notification.h"
*/

/*
void __connect(int clientId);

void __disconnect(int clientId);

void __broadcast(int sender, std::string message);
*/

class Server {
  public:
    Server();
    void start(int port);
    void stop();

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
