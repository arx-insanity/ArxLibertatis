#include "network/cppsockets/TcpClient.h"
#include <vector>

struct Message {
	virtual ~Message() = 0;
	virtual void send(std::vector<unsigned char>& buffer) = 0;
	virtual void read(const unsigned char* buffer, const size_t bufferLen) = 0;

	static void writeSizeT(const size_t size, std::vector<unsigned char>& buffer) {
		size_t writePos = buffer.size();
		buffer.resize(buffer.size() + sizeof(size));
		memcpy(&buffer[writePos], &size, sizeof(size));
	}

	static size_t readSizeT(const unsigned char*& buffer) {
		const size_t len = *reinterpret_cast<const size_t*>(buffer);
		buffer += sizeof(len);
		return len;
	}

	static void writeString(const std::string& str, std::vector<unsigned char>& buffer) {
		size_t len = str.length();
		writeSizeT(len, buffer);
		size_t writePos = buffer.size();
		buffer.resize(buffer.size() + len);
		memcpy(&buffer[writePos], str.c_str(), len);
	}

	static std::string readString(const unsigned char*& buffer) {
		const size_t len = readSizeT(buffer);
		const char* buffer2 = reinterpret_cast<const char*>(buffer);
		buffer += len;
		return std::string(buffer2, len); //might give compiler chance to inline this
	}


};
