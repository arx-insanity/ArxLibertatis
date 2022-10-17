#ifndef NETWORK_MESSAGES_INCOMING_INCOMINGCHATMESSAGE_H
#define NETWORK_MESSAGES_INCOMING_INCOMINGCHATMESSAGE_H
#include "network/messages/incoming/IncomingMessage.h"

struct IncomingChatMessage : public IncomingMessage {
	std::string sender;
	std::string message;

	IncomingChatMessage() {}

	IncomingChatMessage(std::string sender_, std::string message_) : sender(sender), message(message) {}

	virtual MessageType getMessageType() {
		return MessageType::IncomingChatMessage;
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		sender = readString(buffer);
		message = readString(buffer);
	}
};
#endif // NETWORK_MESSAGES_INCOMING_INCOMINGCHATMESSAGE_H
