#ifndef NETWORK_MESSAGES_OUTGOINGCHATMESSAGE_H
#define NETWORK_MESSAGES_OUTGOINGCHATMESSAGE_H
#include "network/messages/Message.h"

struct OutgoingChatMessage : public Message {
	std::string message;

	OutgoingChatMessage() {}

	OutgoingChatMessage(std::string message_) : message(message) {}

	virtual void send(std::vector<unsigned char>& buffer) {
		write(message, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		message = readString(buffer);
	}
};
#endif // NETWORK_MESSAGES_OUTGOINGCHATMESSAGE_H
