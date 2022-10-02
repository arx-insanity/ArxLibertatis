#include "network/messages/Message.h"

struct Handshake : public Message {
	std::string name;

	Handshake() {}

	Handshake(std::string name) : name(name) {

	}

	virtual void send(std::vector<unsigned char>& buffer) {
		size_t len = name.length();
		buffer.resize(len);
		unsigned char* buf = buffer.data();
		memcpy(buf, &len, sizeof(len));
		memcpy(buf + sizeof(len), name.c_str(), len);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		char* buf = (char*)buffer;
		size_t len = *(size_t*)buf;
		name.clear();
		name.reserve(len);
		name.append(buf + sizeof(size_t), len);
	}
private:
	virtual ~Handshake() {

	}
};
