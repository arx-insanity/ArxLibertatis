#ifndef ARX_NETWORK_CLIENT_H
#define ARX_NETWORK_CLIENT_H

#include <thread>
#include <string>

class Client {
  public:
    Client(std::string ip, int port);
    void connect();
    void disconnect();

  private:
    void connectionHandler();
    void write(std::string message);
    std::string read();

    void changeLevel(long int level);

    std::string m_ip;
    int m_port;
    bool m_isRunning;
    int m_socketDescriptor;
    int m_clientDescriptor;
    std::thread * m_thread;
    bool m_isQuitting;
};

#endif // ARX_NETWORK_CLIENT_H
