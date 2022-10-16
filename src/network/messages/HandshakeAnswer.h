#ifndef NETWORK_MESSAGES_HANDSHAKEANSWER_H
#define NETWORK_MESSAGES_HANDSHAKEANSWER_H
#include "network/messages/Message.h"

class HandshakeAnswer : public Message {
	std::string id;
public:
	virtual ~HandshakeAnswer() {

	}

	HandshakeAnswer() {}

	HandshakeAnswer(std::string id) : id(id) {}

	std::string getId() {
		return id;
	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write(id, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		id = readString(buffer);
	}
};
#endif // NETWORK_MESSAGES_HANDSHAKEANSWER_H
