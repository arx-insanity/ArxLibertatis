#ifndef NETWORK_CPPSOCKETS_SOCKET_H
#define NETWORK_CPPSOCKETS_SOCKET_H
#include "network/cppsockets/CppSocketsUtil.h"

namespace CppSockets {
	class Socket {
	private:
		//prevent socket copying
		Socket(const Socket& other) = delete;
		Socket& operator=(const Socket&) = delete;
	protected:
		socket_t _sock;

	public:
		Socket() :_sock(INVALID_SOCKET) {}
		Socket(socket_t sock) :_sock(sock) {}
		virtual ~Socket() {
			close();
		}

		void close(void)
		{
			if (_sock != INVALID_SOCKET) {
#ifdef _WIN32
				closesocket(_sock);
#else
				::close(_sock);
#endif
				_sock = INVALID_SOCKET;
			}
		}
	};

}
#endif // NETWORK_CPPSOCKETS_SOCKET_H
