#ifndef NETWORK_CPPSOCKETS_SOCKET_H
#define NETWORK_CPPSOCKETS_SOCKET_H

#include "network/cppsockets/CppSocketsUtil.h"
#include "platform/Platform.h"

namespace CppSockets {

class Socket {

public:

	Socket() :_sock(INVALID_SOCKET) {}
	Socket(socket_t sock) :_sock(sock) {}

	virtual ~Socket() {
		close();
	}

	void close(void) {
		if (_sock != INVALID_SOCKET) {
			#if ARX_PLATFORM == ARX_PLATFORM_WIN32
				disconnect(_sock, SD_BOTH);
				closesocket(_sock);
			#else
				disconnect(_sock, SHUT_RDWR);
				::close(_sock);
			#endif

			_sock = INVALID_SOCKET;
		}
	}

private:

	//prevent socket copying
	Socket(const Socket& other) = delete;

	Socket& operator=(const Socket&) = delete;

protected:

	socket_t _sock;

};

} // namespace CppSockets

#endif // NETWORK_CPPSOCKETS_SOCKET_H
