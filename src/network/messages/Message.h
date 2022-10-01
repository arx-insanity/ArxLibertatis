#include "network/cppsockets/TcpClient.h"

struct Message {
	virtual ~Message() = 0;
	virtual void send(std::shared_ptr<CppSockets::TcpClient> client)=0;
	virtual void read(std::shared_ptr<unsigned char> buffer) = 0;
};
