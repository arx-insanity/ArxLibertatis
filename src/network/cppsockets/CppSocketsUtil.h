#pragma once

#define CPPSOCKETS_DEBUG

#ifdef CPPSOCKETS_DEBUG
#define CPPSOCKETS_DEBUG_PRINT(...) fprintf(stdout, __VA_ARGS__)
#define CPPSOCKETS_DEBUG_PRINT_ERROR(...) fprintf(stderr, __VA_ARGS__)
#else
#define CPPSOCKETS_DEBUG_PRINT(...)
#define CPPSOCKETS_DEBUG_PRINT_ERROR(...)
#endif

#ifdef _WIN32
// Windows
#pragma comment(lib,"ws2_32.lib")
#define WIN32_LEAN_AND_MEAN
#undef TEXT
#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef SOCKET socket_t;

#else
// Linux
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

#ifdef _WIN32
	void handleWinapiError(int error) {
#ifdef CPPSOCKETS_DEBUG
		LPSTR errorMessagePtr = NULL;
		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT), (LPTSTR)&errorMessagePtr, 0, NULL);
		if (errorMessagePtr) {
			CPPSOCKETS_DEBUG_PRINT_ERROR(errorMessagePtr);
			LocalFree(errorMessagePtr);
		}
		else {
			CPPSOCKETS_DEBUG_PRINT_ERROR("didnt managed to format winapi error %d\n", error);
		}
#endif
	}

	bool _cppsockets_initWinsockSuccess = false;
	bool initWinsock() {
		if (_cppsockets_initWinsockSuccess) {
			return true;
		}
		WSADATA wsaData;
		int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
		if (result != 0) {
			handleWinapiError(result);
			return false;
		}
		_cppsockets_initWinsockSuccess = true;
		return true;
	}

	bool cleanupWinsock() {
		if (_cppsockets_initWinsockSuccess) {
			_cppsockets_initWinsockSuccess = false;
			int result = WSACleanup();
			if (result != 0) {
				handleWinapiError(result);
				return false;
			}
		}
		return true;
	}
#endif

	void cppSocketsInit() {
#ifdef _WIN32
		if (!initWinsock()) {
			//honestly no idea
		}
#endif
	}

	void cppSocketsDeinit() {
#ifdef _WIN32
		if (!cleanupWinsock()) {
			//honestly no idea
		}
#endif
	}

	void inetPton(const char* host, struct sockaddr_in& saddr_in)
	{
#ifdef _WIN32
#ifdef UNICODE
		WCHAR host_[64];
		swprintf_s(host_, L"%S", host);
#else
		const char* host_ = host;
#endif
		InetPton(AF_INET, host_, &(saddr_in.sin_addr.s_addr));
#else
		inet_pton(AF_INET, host, &(saddr_in.sin_addr));
#endif
	}

	void printHex(const char* bytes, size_t len) {
		std::cout << std::hex;
		for (int i = 0; i < len; ++i) {
			std::cout << std::setfill('0') << std::setw(2) << (unsigned int)(unsigned char)bytes[i] << " ";
		}
		std::cout << std::endl;
		std::cout << std::dec;
	}
}