#ifndef ARX_NETWORK_CLIENT_H
#define ARX_NETWORK_CLIENT_H

#include <string>

class Client {
  public:
    Client();
    void connectTo(std::string ip, int port);
};

#endif // ARX_NETWORK_CLIENT_H