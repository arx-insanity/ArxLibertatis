#ifndef ARX_NETWORK_COMMON_H
#define ARX_NETWORK_COMMON_H

#include <string>
#include <stdint.h>

#define EOL "\n"

enum class MessageType :uint16_t {
	//Excplicitly listing ids so they cant be accidentally changed
	Handshake = 1,
	ChatMessage = 2,
	ChangeLevel = 3,
	ServerStopped = 4
};

struct MessagePayloadChangeLevel {
	long levelId;
};

struct FrameHeader {
	uint32_t length;
	uint32_t sender; //TODO: where do i get this id initially? just a random int and hope for no collisions?
	uint16_t messageType;
};

#endif // ARX_NETWORK_COMMON_H
