#include "network/messages/Message.h"

struct Handshake : public Message {
	std::string name;
	Handshake(std::string name) : name(name) {

	}

	virtual void send(std::shared_ptr<CppSockets::TcpClient> client) {
		size_t len = name.length();
		client->sendData(&len, sizeof(len));
		client->sendData(name.c_str(), len);
	}
	virtual void read(std::shared_ptr<unsigned char> buffer) {
		char* buf = (char*)buffer.get();
		size_t len = *(size_t*)buf;
		name.clear();
		name.reserve(len);
		name.append(buf + sizeof(size_t), len);
	}
private:
	virtual ~Handshake() {

	}
};
