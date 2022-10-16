#ifndef ARX_NETWORK_CLIENT_H
#define ARX_NETWORK_CLIENT_H

#include <thread>
#include <string>
#include "network/cppsockets/TcpClient.h"
#include "network/messages/Message.h"

class Client {
	std::string id;
	std::string ip;
	unsigned short port;
	std::shared_ptr<CppSockets::TcpClient> client;
	std::shared_ptr<std::thread> readerThread;
	//std::string nickname;
	std::mutex clientMutex;
	bool readerRunning;

	void readerLoop();
	void handleMessage(MessageType messageType, std::vector<unsigned char> &buffer);

	//no copy or assignment
	Client(const Client& other) = delete;
	Client& operator=(const Client&) = delete;
  public:
    Client(std::string ip, unsigned short port);
    void connect();
    void disconnect();
    bool isConnected();

	void sendMessage(FrameHeader header, unsigned char* body);
	void sendMessage(uint16_t messageType);
	void sendMessage(MessageType messageType);
	void sendMessage(uint16_t messageType, std::vector<unsigned char>& buffer);
	void sendMessage(MessageType messageType, Message* message);
};

#endif // ARX_NETWORK_CLIENT_H
