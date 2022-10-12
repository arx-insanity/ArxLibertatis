#ifndef NETWORK_CPPSOCKETS_CPPSOCKETSUTIL_H
#define NETWORK_CPPSOCKETS_CPPSOCKETSUTIL_H

#include "platform/Platform.h"

#define CPPSOCKETS_DEBUG

#ifdef CPPSOCKETS_DEBUG
	#define CPPSOCKETS_DEBUG_PRINT(...) fprintf(stdout, __VA_ARGS__)
	#define CPPSOCKETS_DEBUG_PRINT_ERROR(...) fprintf(stderr, __VA_ARGS__)
#else
	#define CPPSOCKETS_DEBUG_PRINT(...)
	#define CPPSOCKETS_DEBUG_PRINT_ERROR(...)
#endif

#if ARX_PLATFORM == ARX_PLATFORM_WIN32
	#pragma comment(lib, "ws2_32.lib")
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#undef TEXT
	#include <Windows.h>
	#include <winsock2.h>
	#include <ws2tcpip.h>
	typedef SOCKET socket_t;
#else
	#define sprintf_s sprintf
	typedef int socket_t;
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <unistd.h>
	#include <arpa/inet.h>
	static const socket_t INVALID_SOCKET = ~0;
	static const int SOCKET_ERROR = -1;
#endif

#include <stdio.h>
#include <stdint.h>
#include <mutex>
#include <iostream>
#include <iomanip>

#define LOCK_GUARD(X) std::lock_guard<std::mutex> __lock_guard__(X)

namespace CppSockets {

	#if ARX_PLATFORM == ARX_PLATFORM_WIN32
		void handleWinapiError(int error);

		bool initWinsock();

		bool cleanupWinsock();
	#endif

	void cppSocketsInit();

	void cppSocketsDeinit();

	void inetPton(const char* host, struct sockaddr_in& saddr_in);

	void printHex(const char* bytes, size_t len);

}

#endif // NETWORK_CPPSOCKETS_CPPSOCKETSUTIL_H
