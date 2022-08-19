#ifndef ARX_NETWORK_CLIENT_H
#define ARX_NETWORK_CLIENT_H

#include <string>

class Client {
  public:
    Client(std::string ip, int port);
    void connectTo();
    void disconnect();

  private:
    std::string m_ip;
    int m_port;
};

#endif // ARX_NETWORK_CLIENT_H