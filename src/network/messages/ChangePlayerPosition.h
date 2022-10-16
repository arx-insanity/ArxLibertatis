#ifndef NETWORK_MESSAGES_CHANGEPLAYERPOSITION_H
#define NETWORK_MESSAGES_CHANGEPLAYERPOSITION_H
#include "network/messages/Message.h"
#include "math/Types.h"

struct ChangePlayerPosition : public Message {
  Vec3f position;

  ChangePlayerPosition() {
    position.x = 0;
    position.y = 0;
    position.z = 0;
  }

  ChangePlayerPosition(Vec3f position): position(position) {}

  virtual void send(std::vector<unsigned char>& buffer) {
		write<Vec3f>(position, buffer);
	}

  virtual void read(const unsigned char* buffer, const size_t _) {
		ARX_UNUSED(_);
		position = Message::read<Vec3f>(buffer);
	}
};

#endif // NETWORK_MESSAGES_CHANGEPLAYERPOSITION_H
