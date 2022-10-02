#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/ClientData.h"
#include "network/messages/Handshake.h"

ClientData::ClientData(Server* server, std::shared_ptr<CppSockets::TcpClient> client)
	:server(server), client(client), nickname("unnamed"), readerThread(NULL), readerRunning(false) {
}

ClientData::~ClientData() {
	//TODO: in theory we should thread safely stop the reader here, but thats complicated with sync, so skipping for now
}

std::string ClientData::getNickname() {
	return nickname;
}

std::shared_ptr<CppSockets::TcpClient> ClientData::getClient() {
	return client;
}

Server* ClientData::getServer() {
	return server;
}

void ClientData::startReading() {
	this->readerThread = std::make_shared<std::thread>(&ClientData::readerLoop, this);
}

void ClientData::readerLoop() {
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
		//TODO: move the name assignment to server
		if (frameHeader.messageType == static_cast<uint16_t>(MessageType::Handshake)) {
			Handshake hs;
			hs.read(buffer.data(), buffer.size());
			{
				LOCK_GUARD(clientMutex);
				this->nickname = hs.getName();
			}
		}
		server->handleClientMessage(this, frameHeader.messageType, buffer);
		buffer.clear();
	}
exitReaderLoop:
	{
		LOCK_GUARD(clientMutex);
		readerRunning = false;
	}
}

void ClientData::stopReading() {
	{
		LOCK_GUARD(clientMutex);
		if (!readerRunning) {
			return;
		}
	}
	{
		LOCK_GUARD(clientMutex);
		readerRunning = false;
		client->close();
	}
	readerThread->join();
	{
		LOCK_GUARD(clientMutex);
		readerThread = NULL;
	}
}

void ClientData::sendMessage(FrameHeader header, unsigned char* body) {
	LOCK_GUARD(clientMutex);
	client->sendData(&header, sizeof(header));
	if (body != NULL && header.length > 0) {
		client->sendData(body, header.length);
	}
}
