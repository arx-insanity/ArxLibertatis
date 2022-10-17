#ifndef NETWORK_MESSAGES_INCOMING_HANDSHAKEANSWER_H
#define NETWORK_MESSAGES_INCOMING_HANDSHAKEANSWER_H
#include "network/messages/incoming/IncomingMessage.h"

class HandshakeAnswer : public IncomingMessage {
	std::string id;
public:
	virtual ~HandshakeAnswer() {

	}

	HandshakeAnswer() {}

	HandshakeAnswer(std::string id) : id(id) {}

	std::string getId() {
		return id;
	}

	virtual MessageType getMessageType() {
		return MessageType::HandshakeAnswer;
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		id = readString(buffer);
	}
};
#endif // NETWORK_MESSAGES_INCOMING_HANDSHAKEANSWER_H
