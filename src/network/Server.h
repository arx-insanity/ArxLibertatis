#ifndef ARX_NETWORK_SERVER_H
#define ARX_NETWORK_SERVER_H

#include <thread>
#include <vector>
#include "network/ClientData.h"
#include "network/cppsockets/TcpServer.h"

struct Message;
class ClientData;

class Server {
	std::shared_ptr<CppSockets::TcpServer> tcpServer;
	std::vector<std::shared_ptr<ClientData>> clients;
	std::mutex serverMutex;
	int port;

	void serverAccept(std::shared_ptr<CppSockets::TcpClient> client);

	//no copy and assignment
	Server(const Server& other) = delete;
	Server& operator=(const Server&) = delete;
public:
	Server(int port);
	void start();
	void stop();
	bool isRunning();
	void handleClientMessage(ClientData* sender, MessageType messageType, std::vector<unsigned char>& buffer);
	void broadcast(uint16_t messageType);
	void broadcast(MessageType messageType);
	void broadcast(uint16_t messageType, std::vector<unsigned char>& buffer);
	void broadcast(MessageType messageType, Message* message);
};

#endif // ARX_NETWORK_SERVER_H
