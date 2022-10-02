#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "core/Core.h"
#include "game/Player.h"
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/Client.h"

Client::Client(std::string ip, int port) : ip(ip), port(port) {
	id = rand();
}

void Client::readerLoop() {
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
	case MessageType::ChangeLevel:
		break;
	case MessageType::ChatMessage:
		break;
	case MessageType::ServerStopped:
		break;
	}
}

/*std::string Client::read() {
	if (!this->m_isConnected) {
		LogError << "Can't read remote data, not connected to a server";
		return "";
	}

	MessageType messageType;
	::read(this->m_socketDescriptor, (char*)&messageType, sizeof(messageType));

	if (messageType == MessageTypeServerStopped) {
		return "/quit";
	}

	if (messageType == MessageTypeChangeLevel) {
		char rawInput[10];
		int rawReadSize = ::read(this->m_socketDescriptor, rawInput, 10);
		std::string input(rawInput, rawReadSize);

		return "/changeLevelTo " + input;
	}

	return "";
}

void Client::connectionHandler() {
	this->m_isConnected = true;

	this->m_socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
	if (this->m_socketDescriptor == -1) {
		LogError << "Could not create socket";
		return;
	}

	sockaddr_in client;
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = inet_addr(this->m_ip.c_str());
	client.sin_port = htons(this->m_port);

	this->m_clientDescriptor = ::connect(this->m_socketDescriptor, (sockaddr*)&client, sizeof(client));
	if (this->m_clientDescriptor < 0) {
		LogError << "Could not connect to the server";
		return;
	}

	LogInfo << "Connected";

	do {
		std::string input = this->read();
		if (input != "") {
			if (boost::starts_with(input, "/")) {
				std::string::size_type commandSize = input.find(" ", 0);
				std::string command = input.substr(1, commandSize - 1);
				std::string args = "";
				if (commandSize < input.size() + 1) {
					args = boost::trim_copy(boost::erase_head_copy(input, commandSize));
				}

				LogInfo << "/" + command + " " + args;

				if (command == "quit" || command == "exit") {
					this->m_isQuitting = true;
				}
				else if (command == "changeLevelTo") {
					long int level = strtol(args.c_str(), nullptr, 10);
					this->changeLevel(level);
				}
			}
		}
	} while (!this->m_isQuitting);

	if (this->m_isQuitting) {
		fflush(stdout);
	}

	this->m_isConnected = false;
}*/

bool Client::isConnected() {
	return this->readerRunning;
}

/*void Client::changeLevel(long int level) {
	//guess this is to change level locally
	TELEPORT_TO_LEVEL = level;
	TELEPORT_TO_POSITION = "";
	TELEPORT_TO_ANGLE = static_cast<long>(player.angle.getYaw());
	CHANGE_LEVEL_ICON = ChangeLevelNow;
}*/
