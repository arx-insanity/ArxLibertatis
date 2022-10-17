#ifndef NETWORK_MESSAGES_OUTGOING_OUTGOINGMESSAGE_H
#define NETWORK_MESSAGES_OUTGOING_OUTGOINGMESSAGE_H
#include "network/messages/Message.h"
#include <vector>
#include <cstring>
#include <stdint.h>

struct OutgoingMessage : public Message {
	virtual ~OutgoingMessage() {};
	virtual void send(std::vector<unsigned char>& buffer) = 0;

protected:
	static void write(const std::string& str, std::vector<unsigned char>& buffer) {
		uint32_t len = str.length();
		write(len, buffer);
		if (len > 0) {
			size_t writePos = buffer.size();
			buffer.resize(buffer.size() + len);
			std::memcpy(&buffer[writePos], str.c_str(), len);
		}
	}

	template <typename primT> static void write(primT val, std::vector<unsigned char>& buffer) {
		size_t writePos = buffer.size();
		buffer.resize(buffer.size() + sizeof(val));
		std::memcpy(&buffer[writePos], &val, sizeof(val));
	}
};

#endif // NETWORK_MESSAGES_OUTGOING_OUTGOINGMESSAGE_H
