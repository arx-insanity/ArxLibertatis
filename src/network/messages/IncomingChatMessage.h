#ifndef NETWORK_MESSAGES_INCOMINGCHATMESSAGE_H
#define NETWORK_MESSAGES_INCOMINGCHATMESSAGE_H
#include "network/messages/Message.h"

struct IncomingChatMessage : public Message {
	std::string sender;
	std::string message;

	IncomingChatMessage() {}

	IncomingChatMessage(std::string sender_, std::string message_) : sender(sender), message(message) {}

	virtual void send(std::vector<unsigned char>& buffer) {
		write(sender, buffer);
		write(message, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		sender = readString(buffer);
		message = readString(buffer);
	}
};
#endif // NETWORK_MESSAGES_INCOMINGCHATMESSAGE_H
