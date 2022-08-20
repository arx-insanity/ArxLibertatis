#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <boost/algorithm/string/trim.hpp>
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/Client.h"

Client::Client(std::string ip, int port) {
  this->m_ip = ip;
  this->m_port = port;
  this->m_isRunning = false;
  this->m_isQuitting = false;
}

void Client::connect() {
  if (this->m_isRunning) {
    LogError << CLIENT_PREFIX << "already connected to a server";
    return;
  }

  LogInfo << CLIENT_PREFIX << "connecting to server...";

  this->m_thread = new std::thread(&Client::connectionHandler, this);
}

void Client::disconnect() {
  if (!this->m_isRunning) {
    LogError << CLIENT_PREFIX << "not connected to a server";
    return;
  }

  LogInfo << CLIENT_PREFIX << "disconnecting from server...";

  this->m_isQuitting = true;
  this->m_isRunning = false;
  shutdown(this->m_socketDescriptor, SHUT_RDWR);
  close(this->m_socketDescriptor);

  this->m_thread->join();

  LogInfo << CLIENT_PREFIX << "server stopped";
}

std::string Client::read() {
  char rawInput[2000];
  int rawReadSize = ::read(this->m_socketDescriptor, rawInput, 2000);

  if (rawReadSize == 0) {
    return "/quit";
  }

  // TODO: error handling
  if (rawReadSize < 0) {
    // failed to read
  }

  std::string input(rawInput, rawReadSize);
  boost::trim(input);

  return input;
}

void Client::write(std::string message) {
  ::write(this->m_socketDescriptor, (message + EOL).c_str(), message.size() + 1);
}

void Client::connectionHandler() {
  this->m_isRunning = true;

  this->m_socketDescriptor = socket(AF_INET, SOCK_STREAM, 0);
  if (this->m_socketDescriptor == -1) {
    LogError << CLIENT_PREFIX << "could not create socket";
    return;
  }

  sockaddr_in client;
  client.sin_family = AF_INET;
  client.sin_addr.s_addr = inet_addr(this->m_ip.c_str());
  client.sin_port = htons(this->m_port);

  this->m_clientDescriptor = ::connect(this->m_socketDescriptor, (sockaddr *)&client, sizeof(client));
  if (this->m_clientDescriptor < 0) {
    LogError << CLIENT_PREFIX << "could not connect to the server";
    return;
  }

  LogInfo << SERVER_PREFIX << "connected";

  this->write("/say hello!!!");

  do {
    std::string input = this->read();
    LogInfo << "--- Client: got message from server '" << input << "'";
    if (input == "/quit") {
      this->m_isQuitting = true;
    }
  } while (!this->m_isQuitting);

  if (this->m_isQuitting) {
    fflush(stdout);
  }

  ::close(this->m_clientDescriptor);
}
