#ifndef ARX_NETWORK_COMMON_H
#define ARX_NETWORK_COMMON_H

#include <string>
#include <stdint.h>

enum class MessageType :uint16_t {
	//Excplicitly listing ids so they cant be accidentally changed
	Handshake = 1,
	AnnounceClientEnter = 2,
	AnnounceClientExit = 3,
	AnnounceServerExit = 4,
	LevelChange = 5,
	ByeBye = 6,
	OutgoingChatMessage = 7,
	IncomingChatMessage = 8,
	HandshakeAnswer = 9,
};


#pragma pack(push, 1)

struct FrameHeader {
	uint16_t messageType;
	uint32_t length;
};

#pragma pack(pop)

#endif // ARX_NETWORK_COMMON_H
