#ifndef NETWORK_MESSAGES_OUTGOING_HANDSHAKE_H
#define NETWORK_MESSAGES_OUTGOING_HANDSHAKE_H
#include "network/messages/outgoing/OutgoingMessage.h"

class Handshake : public OutgoingMessage {
	std::string name;
public:
	virtual ~Handshake() {}

	Handshake() {}

	Handshake(std::string name) : name(name) {}

	std::string getName() {
		return name;
	}

	virtual MessageType getMessageType() {
		return MessageType::Handshake;
	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write(name, buffer);
	}
};
#endif // NETWORK_MESSAGES_OUTGOING_HANDSHAKE_H
