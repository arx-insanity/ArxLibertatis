#ifndef NETWORK_CPPSOCKETS_TCPCLIENT_H
#define NETWORK_CPPSOCKETS_TCPCLIENT_H

#include "network/cppsockets/Socket.h"
#include "io/log/Logger.h"
#include <cstring>

namespace CppSockets {
	class TcpClient : public Socket {
	private:
		//prevent socket copying and assignment
		TcpClient(const TcpClient& other) = delete;
		TcpClient& operator=(const TcpClient&) = delete;
	public:

		TcpClient(socket_t sock) :
			Socket(sock) {}

		TcpClient(const char* host, const unsigned short port) :
			Socket()
		{
			//convert port to string cause getaddrinfo wants a string
			char _port[6];
			sprintf(_port, "%d", port);

			// Set up client address info
			struct addrinfo hints = { 0 };
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;

			CPPSOCKETS_DEBUG_PRINT("trying to connect to " << host << ":" << _port);

			// Resolve the server address and port, returning on failure
			addrinfo* _addressInfo = NULL;
			int result = getaddrinfo(host, _port, &hints, &_addressInfo);
			if (result != 0) {
#ifdef _WIN32
				handleWinapiError(result);
#else
				CPPSOCKETS_DEBUG_PRINT_ERROR("getaddrinfo() failed with error: " << result);
#endif
				return;
			}

			CPPSOCKETS_DEBUG_PRINT("creating socket");

			// Create a SOCKET for connecting to server, returning on failure
			_sock = socket(_addressInfo->ai_family, _addressInfo->ai_socktype, _addressInfo->ai_protocol);
			if (_sock == INVALID_SOCKET) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("socket() failed");
				freeaddrinfo(_addressInfo);
				return;
			}

			CPPSOCKETS_DEBUG_PRINT("connecting socket");

			result = connect(_sock, _addressInfo->ai_addr, (int)_addressInfo->ai_addrlen);
			if (result == SOCKET_ERROR) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("connect() failed");
				close();
			}
			freeaddrinfo(_addressInfo);

			CPPSOCKETS_DEBUG_PRINT("finished creating tcp client");
		}

		int sendData(const void* buf, int len)
		{
			return send(_sock, (const char*)buf, len, 0);
		}

		int receiveData(void* buf, int len)
		{
			return recv(_sock, (char*)buf, len, 0);
		}

		template<int size> bool receiveFixedData(void* buf) {
			int left = size;
			unsigned char localBuf[size];
			unsigned char* writePtr = localBuf;
			while (left > 0) {
				int received = receiveData(writePtr, left);
				if (received == 0) {
					return false;
				}
				left -= received;
				writePtr += received;
			}
			std::memcpy(buf, localBuf, size);
			return true;
		}
	};
}

#endif // NETWORK_CPPSOCKETS_TCPCLIENT_H
