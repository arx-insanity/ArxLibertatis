#ifndef NETWORK_MESSAGES_INCOMING_INCOMINGLEVELCHANGE_H
#define NETWORK_MESSAGES_INCOMING_INCOMINGLEVELCHANGE_H
#include "network/messages/incoming/IncomingMessage.h"

struct IncomingLevelChange : public IncomingMessage {
	int32_t level;

	IncomingLevelChange() :level(0) {}

	IncomingLevelChange(int32_t level) : level(level) {}

	virtual MessageType getMessageType() {
		return MessageType::IncomingLevelChange;
	}

	virtual void read(const unsigned char* buffer, const size_t _) {
		ARX_UNUSED(_);
		level = IncomingMessage::read<int32_t>(buffer);
	}
};

#endif // NETWORK_MESSAGES_INCOMING_INCOMINGLEVELCHANGE_H
