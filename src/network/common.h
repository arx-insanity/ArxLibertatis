#ifndef ARX_NETWORK_COMMON_H
#define ARX_NETWORK_COMMON_H

#include <string>

#define EOL "\n"

enum MessageType {
  MessageTypeChangeLevel = 1,
  MessageTypeServerStopped
};

struct MessagePayloadChangeLevel {
  long levelId;
};

#endif // ARX_NETWORK_COMMON_H
