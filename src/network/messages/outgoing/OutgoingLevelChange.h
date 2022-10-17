#ifndef NETWORK_MESSAGES_OUTGOING_OUTGOINGLEVELCHANGE_H
#define NETWORK_MESSAGES_OUTGOING_OUTGOINGLEVELCHANGE_H
#include "network/messages/outgoing/OutgoingMessage.h"

struct OutgoingLevelChange : public OutgoingMessage {
	int32_t level;

	OutgoingLevelChange() :level(0) {}

	OutgoingLevelChange(int32_t level) : level(level) {}

	virtual MessageType getMessageType() {
		return MessageType::OutgoingLevelChange;
	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write<int32_t>(level, buffer);
	}
};

#endif // NETWORK_MESSAGES_OUTGOING_OUTGOINGLEVELCHANGE_H
