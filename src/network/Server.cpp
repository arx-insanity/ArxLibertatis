#include "io/log/Logger.h"
#include "core/GameTime.h"
#include "gui/Notification.h"
#include "network/common.h"
#include "network/Server.h"
#include "network/messages/Message.h"

Server::Server(int port) :port(port) {}

void Server::start() {
	if (isRunning()) {
		LogError << "Can't start server, already started";
		return;
	}

	LogInfo << "Server starting...";

	tcpServer = std::make_shared<CppSockets::TcpServer>(port);
	tcpServer->acceptCallback = std::bind(std::mem_fn(&Server::serverAccept), this, std::placeholders::_1);
	tcpServer->startListening();
}

void Server::stop() {
	if (!isRunning()) {
		LogError << "Can't stop server, not running";
		return;
	}

	LogInfo << "Stopping server...";

	//TODO: tell all clients server stopped

	tcpServer->stopListening();

	LogInfo << "Server stopped";
}

void Server::serverAccept(std::shared_ptr<CppSockets::TcpClient> client) {
	LOCK_GUARD(serverMutex);
	std::shared_ptr<ClientData> cd = std::make_shared<ClientData>(this, client);
	clients.push_back(cd);
}


bool Server::isRunning() {
	return this->tcpServer->isListening();
}

void Server::broadcast(uint32_t sender, uint16_t messageType) {
	FrameHeader fh;
	fh.sender = sender;
	fh.messageType = messageType;
	fh.length = 0;
	for (std::shared_ptr<ClientData> client : clients) {
		client->sendMessage(fh, NULL, 0);
	}
}
void Server::broadcast(uint32_t sender, uint16_t messageType, std::vector<unsigned char>& buffer) {
	FrameHeader fh;
	fh.sender = sender;
	fh.messageType = messageType;
	fh.length = buffer.size();
	for (std::shared_ptr<ClientData> client : clients) {
		client->sendMessage(fh, buffer.data(), buffer.size());
	}
}

void Server::broadcast(uint32_t sender, MessageType messageType, Message* message) {
	std::vector<unsigned char> buffer;
	message->send(buffer);
	broadcast(sender, static_cast<uint16_t>(messageType), buffer);
}

void Server::handleClientMessage(ClientData* sender, uint16_t messageType, std::vector<unsigned char>& buffer) {
	//TODO: this is going to be one ugly switch case for now
}
