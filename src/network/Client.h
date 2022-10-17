#ifndef ARX_NETWORK_CLIENT_H
#define ARX_NETWORK_CLIENT_H

#include <thread>
#include <string>
#include "network/cppsockets/TcpClient.h"
#include "network/messages/Message.h"
#include "network/messages/outgoing/OutgoingMessage.h"
#include "network/messages/incoming/IncomingMessage.h"

class Client {
	std::string id;
	std::string server_ip;
	unsigned short server_port;
	std::shared_ptr<CppSockets::TcpClient> client;
	std::shared_ptr<std::thread> readerThread;
	//std::string nickname;
	std::mutex clientMutex;
	bool readerRunning;

	void readerLoop();
	void handleMessage(IncomingMessage* message);

	//no copy or assignment
	Client(const Client& other) = delete;
	Client& operator=(const Client&) = delete;
  public:
    Client(std::string server_ip, unsigned short server_port);
    void connect();
    void disconnect();
    bool isConnected();

	void sendMessage(OutgoingMessage* message);
};

#endif // ARX_NETWORK_CLIENT_H
