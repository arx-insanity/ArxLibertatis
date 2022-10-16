#ifndef NETWORK_MESSAGES_LEVELCHANGE_H
#define NETWORK_MESSAGES_LEVELCHANGE_H
#include "network/messages/Message.h"

struct LevelChange : public Message {
	int32_t level;

	LevelChange() :level(0) {}

	LevelChange(int32_t level) : level(level) {}

	virtual void send(std::vector<unsigned char>& buffer) {
		write<int32_t>(level, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t _) {
		ARX_UNUSED(_);
		level = Message::read<int32_t>(buffer);
	}
};

#endif // NETWORK_MESSAGES_LEVELCHANGE_H
