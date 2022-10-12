#include "network/cppsockets/CppSocketsUtil.h"
#include "platform/Platform.h"

namespace CppSockets {

	#if ARX_PLATFORM == ARX_PLATFORM_WIN32
		void handleWinapiError(int error) {
			#ifdef CPPSOCKETS_DEBUG
				LPSTR errorMessagePtr = NULL;
				FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
					NULL,
					error,
					MAKELANGID(LANG_ENGLISH, SUBLANG_DEFAULT),
					(LPTSTR)&errorMessagePtr,
					0,
					NULL
				);

				if (errorMessagePtr) {
					CPPSOCKETS_DEBUG_PRINT_ERROR(errorMessagePtr);
					LocalFree(errorMessagePtr);
				} else {
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
		#if ARX_PLATFORM == ARX_PLATFORM_WIN32
			if (!initWinsock()) {
				//honestly no idea
			}
		#endif
	}

	void cppSocketsDeinit() {
		#if ARX_PLATFORM == ARX_PLATFORM_WIN32
			if (!cleanupWinsock()) {
				//honestly no idea
			}
		#endif
	}

	void inetPton(const char* host, struct sockaddr_in& saddr_in) {
		#if ARX_PLATFORM == ARX_PLATFORM_WIN32
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
