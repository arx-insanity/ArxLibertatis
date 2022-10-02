#ifndef ARX_NETWORK_CLIENT_DATA_H
#define ARX_NETWORK_CLIENT_DATA_H

#include <string>
#include <thread>
#include <mutex>
#include "game/Entity.h"
#include "network/common.h"
#include "network/Server.h"
#include "network/cppsockets/TcpClient.h"

class Server;

class ClientData {
	Server* server;
	std::shared_ptr<CppSockets::TcpClient> client;
	std::shared_ptr<std::thread> readerThread;
	std::string nickname;
	std::mutex clientMutex;
	uint32_t id;
	bool readerRunning;

	void readerLoop();

	//no copy or assignment
	ClientData(const ClientData& other) = delete;
	ClientData& operator=(const ClientData&) = delete;
public:
	ClientData(Server* server, std::shared_ptr<CppSockets::TcpClient> client);
	~ClientData();
	std::string getNickname();
	void setNickname(std::string name);
	uint32_t getId();
	std::shared_ptr<CppSockets::TcpClient> getClient();
	Server* getServer();
	void startReading();
	void stopReading();
	void sendMessage(FrameHeader header, unsigned char* body);
};

#endif // ARX_NETWORK_CLIENT_DATA_H
