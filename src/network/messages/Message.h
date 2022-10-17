#ifndef NETWORK_MESSAGES_MESSAGE_H
#define NETWORK_MESSAGES_MESSAGE_H
#include <stdint.h>
#include "network/common.h"

struct Message {
	virtual ~Message() = default;
	virtual MessageType getMessageType() = 0;
};

#endif // NETWORK_MESSAGES_MESSAGE_H
