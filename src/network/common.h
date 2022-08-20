#ifndef ARX_NETWORK_COMMON_H
#define ARX_NETWORK_COMMON_H

#include <string>

#define EOL "\n"

const std::string CLIENT_PREFIX = "Arx Client: ";
const std::string SERVER_PREFIX = "Arx Server: ";

enum MessageType {
  MessageTypeLoadLevel = 1,
  MessageTypeServerStopped
};

struct MessageLoadLevel {
  unsigned char levelId;
};

enum DisconnectType {
  DisconnectTypeServerStopped,
  DisconnectTypeClientDisconnected,
};

struct MessageDisconnected {
  DisconnectType reason;
};

#endif // ARX_NETWORK_COMMON_H
