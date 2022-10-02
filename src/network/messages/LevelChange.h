#include "network/messages/Message.h"

struct LevelChange : public Message {
	long level;

	LevelChange() {}

	LevelChange(long level) : level(level) {

	}

	virtual void send(std::vector<unsigned char>& buffer) {
		write<long>(level, buffer);
	}

	virtual void read(const unsigned char* buffer, const size_t bufferLen) {
		level = Message::read<long>(buffer);
	}
};
