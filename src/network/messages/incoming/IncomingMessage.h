#ifndef NETWORK_MESSAGES_INCOMING_INCOMINGMESSAGE_H
#define NETWORK_MESSAGES_INCOMING_INCOMINGMESSAGE_H
#include "network/messages/Message.h"
#include <vector>
#include <cstring>
#include <stdint.h>

struct IncomingMessage : public Message {
	virtual ~IncomingMessage() {};
	virtual void read(const unsigned char* buffer, const size_t bufferLen) = 0;

protected:
	static std::string readString(const unsigned char*& buffer) {
		const auto len = read<uint32_t>(buffer);
		const char* buffer2 = reinterpret_cast<const char*>(buffer);
		buffer += len;
		return std::string(buffer2, len); //might give compiler chance to inline this
	}

	template <typename primT> static primT read(const unsigned char*& buffer) {
		const primT val = *reinterpret_cast<const primT*>(buffer);
		buffer += sizeof(val);
		return val;
	}
};

#endif // NETWORK_MESSAGES_INCOMING_INCOMINGMESSAGE_H
