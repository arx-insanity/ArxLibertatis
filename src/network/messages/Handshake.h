#ifndef NETWORK_MESSAGES_HANDSHAKE_H
#define NETWORK_MESSAGES_HANDSHAKE_H
#include "network/messages/Message.h"

class Handshake : public Message {
	std::string name;
public:
	virtual ~Handshake() {

	}
	Handshake() {}
	Handshake(std::string name) : name(name) {

	}

	std::string getName() {
		return name;
	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write(name, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		name = readString(buffer);
	}
};
#endif // NETWORK_MESSAGES_HANDSHAKE_H
