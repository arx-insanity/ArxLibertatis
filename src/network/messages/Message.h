#ifndef NETWORK_MESSAGES_MESSAGE_H
#define NETWORK_MESSAGES_MESSAGE_H
#include "network/cppsockets/TcpClient.h"
#include <vector>
#include <cstring>

struct Message {
	virtual ~Message() {};
	virtual void send(std::vector<unsigned char>& buffer) = 0;
	virtual void read(const unsigned char* buffer, const size_t bufferLen) = 0;

protected:
	static void write(const std::string& str, std::vector<unsigned char>& buffer) {
		size_t len = str.length();
		write<size_t>(len, buffer);
		size_t writePos = buffer.size();
		buffer.resize(buffer.size() + len);
		std::memcpy(&buffer[writePos], str.c_str(), len);
	}

	static std::string readString(const unsigned char*& buffer) {
		const size_t len = read<size_t>(buffer);
		const char* buffer2 = reinterpret_cast<const char*>(buffer);
		buffer += len;
		return std::string(buffer2, len); //might give compiler chance to inline this
	}

	template <typename primT> static void write(primT val, std::vector<unsigned char>& buffer) {
		size_t writePos = buffer.size();
		buffer.resize(buffer.size() + sizeof(val));
		std::memcpy(&buffer[writePos], &val, sizeof(val));
	}

	template <typename primT> static primT read(const unsigned char*& buffer) {
		const primT val = *reinterpret_cast<const primT*>(buffer);
		buffer += sizeof(val);
		return val;
	}
};

#endif // NETWORK_MESSAGES_MESSAGE_H
