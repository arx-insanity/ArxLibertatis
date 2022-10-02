#include "network/cppsockets/TcpClient.h"
#include <vector>

struct Message {
	virtual ~Message() = 0;
	virtual void send(std::vector<unsigned char>& buffer) = 0;
	virtual void read(const unsigned char* buffer, const size_t bufferLen) = 0;
};
