#ifndef NETWORK_MESSAGES_INCOMING_INCOMINGCHANGEPLAYERPOSITION_H
#define NETWORK_MESSAGES_INCOMING_INCOMINGCHANGEPLAYERPOSITION_H
#include "network/messages/incoming/IncomingMessage.h"
#include "math/Types.h"

struct IncomingChangePlayerPosition : public IncomingMessage {
	std::string sender_id;
	Vec3f position;

	IncomingChangePlayerPosition() {
		position.x = 0;
		position.y = 0;
		position.z = 0;
	}

	virtual MessageType getMessageType() {
		return MessageType::IncomingChangePlayerPosition;
	}

	virtual void read(const unsigned char* buffer, const size_t _) {
		ARX_UNUSED(_);
		sender_id = readString(buffer);
		position = IncomingMessage::read<Vec3f>(buffer);
	}
};

#endif // NETWORK_MESSAGES_INCOMING_INCOMINGCHANGEPLAYERPOSITION_H
