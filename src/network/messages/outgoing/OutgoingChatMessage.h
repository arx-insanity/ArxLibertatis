#ifndef NETWORK_MESSAGES_OUTGOING_OUTGOINGCHATMESSAGE_H
#define NETWORK_MESSAGES_OUTGOING_OUTGOINGCHATMESSAGE_H
#include "network/messages/outgoing/OutgoingMessage.h"

struct OutgoingChatMessage : public OutgoingMessage {
	std::string message;

	OutgoingChatMessage() {}

	OutgoingChatMessage(std::string message_) : message(message) {}

	virtual MessageType getMessageType() {
		return MessageType::OutgoingChatMessage;
	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write(message, buffer);
	}
};
#endif // NETWORK_MESSAGES_OUTGOING_OUTGOINGCHATMESSAGE_H
