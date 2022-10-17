#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "core/Core.h"
#include "game/Player.h"
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/Client.h"
#include "network/messages/MessageFactory.h"

Client::Client(std::string server_ip, unsigned short server_port) : server_ip(server_ip), server_port(server_port), readerRunning(false) {
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
		IncomingMessage* message = static_cast<IncomingMessage*>(MessageFactory::createMessage(static_cast<MessageType>(frameHeader.messageType)));
		message->read(buffer.data(), buffer.size());
		handleMessage(message);
		delete message;
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
	client = std::make_shared<CppSockets::TcpClient>(server_ip.c_str(), server_port);
	this->readerThread = std::make_shared<std::thread>(&Client::readerLoop, this);

	Handshake msg("Am Shaegar");
	this->sendMessage(&msg);

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

void Client::handleMessage(IncomingMessage* message) {
	MessageType messageType = message->getMessageType();
	switch (messageType) {
	case MessageType::HandshakeAnswer:
	{
		auto handshakeAnswer = static_cast<HandshakeAnswer*>(message);
		id = handshakeAnswer->getId();
		break;
	}
	case MessageType::IncomingLevelChange:
	{
		auto incominglevelChange = static_cast<IncomingLevelChange*>(message);
		TELEPORT_TO_LEVEL = incominglevelChange->level;
		TELEPORT_TO_POSITION = "";
		TELEPORT_TO_ANGLE = static_cast<long>(player.angle.getYaw());
		CHANGE_LEVEL_ICON = ChangeLevelNow;
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
	case MessageType::IncomingChangePlayerPosition:
	{
		auto incomingChangePlayerPosition = static_cast<IncomingChangePlayerPosition*>(message);
		std::string clientId = incomingChangePlayerPosition->sender_id;
		auto pos = incomingChangePlayerPosition->position;
		LogInfo << "client " << clientId << " moved to: [" << pos.x << ", " << pos.y << ", " << pos.z << "]";
		break;
	}
	}
}

bool Client::isConnected() {
	return this->readerRunning;
}

void Client::sendMessage(OutgoingMessage* message) {
	std::vector<unsigned char> buffer;
	message->send(buffer);
	FrameHeader fh{};
	fh.length = buffer.size();
	fh.messageType = static_cast<uint16_t>(message->getMessageType());
	client->sendData(&fh, sizeof(fh));
	if (fh.length > 0) {
		auto body = buffer.data();
		client->sendData(body, fh.length);
	}
}


