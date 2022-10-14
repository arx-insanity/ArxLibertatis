#ifndef NETWORK_CPPSOCKETS_TCPSERVER_H
#define NETWORK_CPPSOCKETS_TCPSERVER_H

#include "network/cppsockets/TcpClient.h"
#include "network/cppsockets/Socket.h"
#include "network/cppsockets/CppSocketsUtil.h"
#include "platform/Platform.h"
#include "io/log/Logger.h"
#include <memory>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <functional>

#include <iostream>
#include <iomanip>
#if ARX_PLATFORM != ARX_PLATFORM_WIN32
#include <fcntl.h>
#include <poll.h>
#endif

namespace CppSockets {

	class TcpServer :public Socket {
		int backlog;
		unsigned short port;
		std::shared_ptr<std::thread> listenerThread;
		volatile bool listenerRunning;
		volatile bool listening;
		std::mutex serverMutex;

		//prevent socket copying and assignment
		TcpServer(const TcpServer& other) = delete;
		TcpServer& operator=(const TcpServer&) = delete;
	public:
		std::function<void(std::shared_ptr<TcpClient>)> acceptCallback;

		TcpServer(unsigned short port, int backlog = 0x7fffffff)
			:Socket(), port(port), backlog(backlog), listenerThread(NULL), listenerRunning(false), listening(false), serverMutex(), acceptCallback(0)
		{}

		bool isListening() {
			return listening;
		}

		void startListening() {
			LOCK_GUARD(serverMutex);
			if (listening) {
				return;
			}
			//convert port to string cause getaddrinfo wants a string
			char _port[6];
			sprintf(_port, "%d", port);

			struct addrinfo hints;
			hints.ai_flags = AI_PASSIVE;
			hints.ai_family = AF_INET;
			hints.ai_socktype = SOCK_STREAM;
			hints.ai_protocol = IPPROTO_TCP;
			hints.ai_addrlen = 0;
			hints.ai_canonname = NULL;
			hints.ai_addr = NULL;
			hints.ai_next = NULL;

			// Resolve the server address and port
			addrinfo* _addressInfo = NULL;
			int result = getaddrinfo(NULL, _port, &hints, &_addressInfo);
			if (result != 0) {
#if ARX_PLATFORM == ARX_PLATFORM_WIN32
				handleWinapiError(result);
#else
				CPPSOCKETS_DEBUG_PRINT_ERROR("getaddrinfo() failed with error: %d", result);
#endif
				return;
			}

			// Create a SOCKET for the server to listen for client connections.
			_sock = socket(_addressInfo->ai_family, _addressInfo->ai_socktype, _addressInfo->ai_protocol);
			if (_sock == INVALID_SOCKET) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("socket() failed");
				freeaddrinfo(_addressInfo);
				return;
			}

#if ARX_PLATFORM == ARX_PLATFORM_WIN32
			u_long mode = 1; //0 for blocking, 1 for non blocking
			result = ioctlsocket(_sock, FIONBIO, &mode);
			if (result != NO_ERROR) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("ioctlsocket failed with error: " << result);
			}
#else
			int flags = fcntl(_sock, F_GETFL, 0);
			if (flags == -1) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("fcntl failed to get flags");
			}
			flags |= O_NONBLOCK;
			if (fcntl(_sock, F_SETFL, flags) != 0) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("fcntl failed to set flags");
			}
#endif

			// Setup the TCP listening socket
			result = bind(_sock, _addressInfo->ai_addr, (int)_addressInfo->ai_addrlen);
			if (result == SOCKET_ERROR) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("bind() failed");
				freeaddrinfo(_addressInfo);
				close();
				return;
			}

			freeaddrinfo(_addressInfo);

			result = listen(_sock, backlog);
			if (result == SOCKET_ERROR) {
				CPPSOCKETS_DEBUG_PRINT_ERROR("listen() failed");
				close();
				return;
			}

			listening = true;
			listenerRunning = true;
			//start thread that runs listenLoop
			listenerThread = std::make_shared<std::thread>(&TcpServer::listenLoop, this);
		}

		void stopListening() {
			{
				LOCK_GUARD(serverMutex);
				if (!listening) {
					return;
				}
				listenerRunning = false;
			}
			listenerThread->join();
			{
				LOCK_GUARD(serverMutex);
				listening = false;
			}
		}

	private:
		void listenLoop() {
#if ARX_PLATFORM == ARX_PLATFORM_WIN32
			struct timeval timeout;
			timeout.tv_sec = 1;  // 1s timeout
			timeout.tv_usec = 0;

			fd_set read_fds;
			while (listenerRunning) {
				FD_ZERO(&read_fds);
				FD_SET(_sock, &read_fds);
				//accept doesnt have a timeout, use select to check if new connections are available
				int select_status = select(NULL, &read_fds, NULL, NULL, &timeout); //0 if timeout
				if (select_status == -1) {
					// ERROR: do something
					CPPSOCKETS_DEBUG_PRINT_ERROR("select() failed " << WSAGetLastError());
					break;
				}
				else if (select_status > 0) {
					socket_t _conn = accept(_sock, (struct sockaddr*)NULL, NULL);
					if (_conn == INVALID_SOCKET) {
						CPPSOCKETS_DEBUG_PRINT_ERROR("accept() failed");
						break;
					}
					std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(_conn);
					// call function with client
					if (acceptCallback) {
						acceptCallback(client);
					}
					else {
						client->close(); //prevent leak i guess?
					}
				}
			}
#else
			pollfd pollfds[1];
			pollfds[0].fd = _sock;
			while (listenerRunning) {
				pollfds[0].revents = 0;
				pollfds[0].events = POLLIN;

				if (poll(pollfds, 1, 1) < 0) {
					CPPSOCKETS_DEBUG_PRINT_ERROR("poll() failed");
				}

				/* Handle an incoming connection. */
				if (pollfds[0].revents & POLLERR) {
					CPPSOCKETS_DEBUG_PRINT_ERROR("server failed");
					break;
				}
				else if (pollfds[0].revents & POLLIN) {
					socket_t _conn = accept(_sock, (struct sockaddr*)NULL, NULL);
					if (_conn < 0) {
						CPPSOCKETS_DEBUG_PRINT_ERROR("accept() failed");
						break;
					}
					std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(_conn);
					//call function with client
					if (acceptCallback) {
						acceptCallback(client);
					}
					else {
						client->close(); //prevent leak i guess?
					}
				}
			}
#endif
		}
	};
}

#endif NETWORK_CPPSOCKETS_TCPSERVER_H
