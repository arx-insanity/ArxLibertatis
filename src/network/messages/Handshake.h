#include "network/messages/Message.h"

struct Handshake : public Message {
	std::string name;

	Handshake() {}

	Handshake(std::string name) : name(name) {

	}

	virtual void send(std::vector<unsigned char>& buffer) {
		writeString(name, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		name = readString(buffer);
	}
private:
	virtual ~Handshake() {

	}
};
