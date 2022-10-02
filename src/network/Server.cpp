#include "io/log/Logger.h"
#include "core/GameTime.h"
#include "gui/Notification.h"
#include "network/common.h"
#include "network/Server.h"
#include "network/messages/Message.h"
#include "network/messages/Handshake.h"
#include "network/messages/LevelChange.h"

Server::Server(int port) :port(port) {
	LogInfo << "Server constructed";
}

void Server::start() {
	if (isRunning()) {
		LogError << "Can't start server, already started";
		return;
	}

	LogInfo << "Server starting...";
	LogInfo << "Sockets Init";
	CppSockets::cppSocketsInit();
	LogInfo << "Create Tcp Server";
	tcpServer = std::make_shared<CppSockets::TcpServer>(port);
	LogInfo << "Set Accept Callback";
	tcpServer->acceptCallback = std::bind(std::mem_fn(&Server::serverAccept), this, std::placeholders::_1);
	LogInfo << "Start Listening";
	tcpServer->startListening();
	LogInfo << "Server Started";
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
	LogInfo << "Client Connecting...";
	LOCK_GUARD(serverMutex);
	std::shared_ptr<ClientData> cd = std::make_shared<ClientData>(this, client);
	clients.push_back(cd);
}


bool Server::isRunning() {
	return tcpServer != NULL && tcpServer->isListening();
}

void Server::broadcast(uint32_t sender, uint16_t messageType) {
	FrameHeader fh;
	fh.sender = sender;
	fh.messageType = messageType;
	fh.length = 0;
	for (std::shared_ptr<ClientData> client : clients) {
		client->sendMessage(fh, NULL);
	}
}

void Server::broadcast(uint32_t sender, MessageType messageType) {
	broadcast(sender, static_cast<uint16_t>(messageType));
}

void Server::broadcast(uint32_t sender, uint16_t messageType, std::vector<unsigned char>& buffer) {
	FrameHeader fh;
	fh.sender = sender;
	fh.messageType = messageType;
	fh.length = buffer.size();
	for (std::shared_ptr<ClientData> client : clients) {
		client->sendMessage(fh, buffer.data());
	}
}

void Server::broadcast(uint32_t sender, MessageType messageType, Message* message) {
	std::vector<unsigned char> buffer;
	message->send(buffer);
	broadcast(sender, static_cast<uint16_t>(messageType), buffer);
}

void Server::handleClientMessage(ClientData* sender, MessageType messageType, std::vector<unsigned char>& buffer) {
	//TODO: this is going to be one ugly switch case for now
	LogInfo << "Getting message from client " << sender->getId() << " message type: " << static_cast<uint16_t>(messageType);
	switch (messageType) {
	case MessageType::Handshake:
	{
		Handshake hs;
		hs.read(buffer.data(), buffer.size());
		{
			sender->setNickname(hs.getName());
		}
		break;
	}
	case MessageType::ChangeLevel:
	{
		LevelChange msg;
		msg.read(buffer.data(), buffer.size());
		broadcast(sender->getId(), static_cast<uint16_t>(messageType), buffer);
		break;
	}
	case MessageType::ChatMessage:
		break;
	case MessageType::ServerStopped:
		break;
	}
}
