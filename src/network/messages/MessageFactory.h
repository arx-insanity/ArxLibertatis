#ifndef NETWORK_MESSAGES_MESSAGEFACTORY_H
#define NETWORK_MESSAGES_MESSAGEFACTORY_H
#include <memory>
#include "network/common.h"
#include "network/messages/Message.h"
//incoming
#include "network/messages/incoming/HandshakeAnswer.h"
#include "network/messages/incoming/IncomingChangePlayerPosition.h"
#include "network/messages/incoming/IncomingChatMessage.h"
#include "network/messages/incoming/IncomingLevelChange.h"
//outgoing
#include "network/messages/outgoing/Handshake.h"
#include "network/messages/outgoing/OutgoingChangePlayerPosition.h"
#include "network/messages/outgoing/OutgoingChatMessage.h"
#include "network/messages/outgoing/OutgoingLevelChange.h"

#define MESSAGE_FACTORY_SHORTHAND(X) case MessageType::X: return new X();break

struct MessageFactory {
	static Message* createMessage(uint16_t messageType) {
		return createMessage(static_cast<MessageType>(messageType));
	}

	static Message* createMessage(MessageType messageType) {
		switch (messageType) {
			//incoming
			MESSAGE_FACTORY_SHORTHAND(HandshakeAnswer);
			MESSAGE_FACTORY_SHORTHAND(IncomingChangePlayerPosition);
			MESSAGE_FACTORY_SHORTHAND(IncomingChatMessage);
			MESSAGE_FACTORY_SHORTHAND(IncomingLevelChange);
			//outgoing
			MESSAGE_FACTORY_SHORTHAND(Handshake);
			MESSAGE_FACTORY_SHORTHAND(OutgoingChangePlayerPosition);
			MESSAGE_FACTORY_SHORTHAND(OutgoingChatMessage);
			MESSAGE_FACTORY_SHORTHAND(OutgoingLevelChange);
		}
	}
};

#endif // NETWORK_MESSAGES_MESSAGEFACTORY_H
