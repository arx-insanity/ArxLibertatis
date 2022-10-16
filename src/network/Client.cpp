#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "core/Core.h"
#include "game/Player.h"
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/Client.h"
#include "network/messages/LevelChange.h"
#include "network/messages/Handshake.h"
#include "network/messages/HandshakeAnswer.h"

Client::Client(std::string ip, unsigned short port) : ip(ip), port(port), readerRunning(false) {
	id = ""; //gets set by handshake answer
}

void Client::readerLoop() {
	this->readerRunning = true;

	const uint32_t bufferSize = 1024;
	std::vector<unsigned char> buffer;
	unsigned char constBuffer[bufferSize];
	while (this->readerRunning) {
		//read frame header
		if (!client->receiveFixedData<sizeof(FrameHeader)>(constBuffer)) {
			break; //closed
		}
		FrameHeader frameHeader = *reinterpret_cast<FrameHeader*>(constBuffer);
		//read message body into buffer
		uint32_t left = frameHeader.length;
		while (left > 0) {
			int received = client->receiveData(constBuffer, std::min(bufferSize, left));
			if (received == 0) {
				goto exitReaderLoop; //closed
			}
			left -= received;
			size_t writePos = buffer.size();
			buffer.resize(writePos + received);
			memcpy(&buffer[writePos], constBuffer, received);
		}
		handleMessage(static_cast<MessageType>(frameHeader.messageType), buffer);
		buffer.clear();
	}
exitReaderLoop:
	{
		LOCK_GUARD(clientMutex);
		readerRunning = false;
	}
}

void Client::connect() {
	if (this->isConnected()) {
		LogError << "Can't connect to a server, already connected";
		return;
	}
	LogInfo << "Connecting to server...";
	CppSockets::cppSocketsInit();
	client = std::make_shared<CppSockets::TcpClient>(ip.c_str(), port);
	this->readerThread = std::make_shared<std::thread>(&Client::readerLoop, this);

	Handshake msg("Am Shaegar");
	this->sendMessage(MessageType::Handshake, &msg);

	LogInfo << "Connected";
}

void Client::disconnect() {
	if (!this->isConnected()) {
		LogError << "Can't disconnect from server, not connected";
		return;
	}
	LogInfo << "Disconnecting from server...";
	client->close();
	readerThread->join();
	LogInfo << "Disconnected";
}

void Client::handleMessage(MessageType messageType, std::vector<unsigned char>& buffer) {
	switch (messageType) {
	case MessageType::LevelChange:
	{
		LevelChange msg;
		msg.read(buffer.data(), buffer.size());
		TELEPORT_TO_LEVEL = msg.level;
		TELEPORT_TO_POSITION = "";
		TELEPORT_TO_ANGLE = static_cast<long>(player.angle.getYaw());
		CHANGE_LEVEL_ICON = ChangeLevelNow;
		break;
	}
	case MessageType::HandshakeAnswer:
	{
		HandshakeAnswer msg;
		msg.read(buffer.data(), buffer.size());
		id = msg.getId();
		break;
	}
	case MessageType::IncomingChatMessage:
		//TODO: display chat message
		break;
	case MessageType::AnnounceClientEnter:
		//TODO: add other player to our game
		break;
	case MessageType::AnnounceClientExit:
		//TODO: remove other player from our game
		break;
	case MessageType::AnnounceServerExit:
		//TODO: disconnect
		break;
	}
}

bool Client::isConnected() {
	return this->readerRunning;
}

void Client::sendMessage(FrameHeader header, unsigned char* body) {
	client->sendData(&header, sizeof(header));
	if (header.length > 0 && body != NULL) {
		client->sendData(body, header.length);
	}
}
void Client::sendMessage(uint16_t messageType) {
	FrameHeader fh;
	fh.length = 0;
	fh.messageType = messageType;
	sendMessage(fh, NULL);
}
void Client::sendMessage(MessageType messageType) {
	sendMessage(static_cast<uint16_t>(messageType));
}

void Client::sendMessage(uint16_t messageType, std::vector<unsigned char>& buffer) {
	FrameHeader fh;
	fh.length = buffer.size();
	fh.messageType = messageType;
	sendMessage(fh, buffer.data());
}
void Client::sendMessage(MessageType messageType, Message* message) {
	std::vector<unsigned char> buffer;
	message->send(buffer);
	sendMessage(static_cast<uint16_t>(messageType), buffer);
}

