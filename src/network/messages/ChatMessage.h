#ifndef NETWORK_MESSAGES_CHATMESSAGE_H
#define NETWORK_MESSAGES_CHATMESSAGE_H
#include "network/messages/Message.h"

struct ChatMessage : public Message {
	std::string sender;
	std::string message;

	ChatMessage() {}

	ChatMessage(std::string sender, std::string message) : sender(sender), message(message) {

	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write(sender, buffer);
		write(message, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		sender = readString(buffer);
		message = readString(buffer);
	}
};
#endif // NETWORK_MESSAGES_CHATMESSAGE_H
