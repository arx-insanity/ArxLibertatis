#ifndef ARX_NETWORK_COMMON_H
#define ARX_NETWORK_COMMON_H

#include <string>
#include <stdint.h>

#define EOL "\n"

enum class MessageType :uint16_t {
	//Excplicitly listing ids so they cant be accidentally changed
	Handshake = 1,
	AnnounceClientEnter = 2,
	AnnounceClientExit = 3,
	AnnounceServerExit = 4,
	LevelChange = 5,
	ByeBye = 6,
	ChatMessage = 7,
};

struct MessagePayloadChangeLevel {
	long levelId;
};

struct FrameHeader {
	uint16_t messageType;
	uint32_t length;
};

#endif // ARX_NETWORK_COMMON_H
