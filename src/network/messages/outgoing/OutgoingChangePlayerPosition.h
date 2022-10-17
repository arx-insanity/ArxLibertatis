#ifndef NETWORK_MESSAGES_OUTGOING_OUTGOINGCHANGEPLAYERPOSITION_H
#define NETWORK_MESSAGES_OUTGOING_OUTGOINGCHANGEPLAYERPOSITION_H
#include "network/messages/outgoing/OutgoingMessage.h"
#include "math/Types.h"

struct OutgoingChangePlayerPosition : public OutgoingMessage {
	Vec3f position;

	OutgoingChangePlayerPosition() {
		position.x = 0;
		position.y = 0;
		position.z = 0;
	}

	OutgoingChangePlayerPosition(Vec3f position) : position(position) {}

	virtual MessageType getMessageType() {
		return MessageType::OutgoingChangePlayerPosition;
	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write<Vec3f>(position, buffer);
	}
};

#endif // NETWORK_MESSAGES_OUTGOING_OUTGOINGCHANGEPLAYERPOSITION_H
