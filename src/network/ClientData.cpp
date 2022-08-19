#include <sys/socket.h>
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/ClientData.h"

std::thread m_client_thread;

ClientData::ClientData(int descriptor) {
  this->m_descriptor = descriptor;
  this->m_nickname = "client #" + descriptor;
  this->m_entity = nullptr;
}

int ClientData::getDescriptor() {
  return this->m_descriptor;
}

void ClientData::write(std::string message) {
  ::write(this->m_descriptor, (message + EOL).c_str(), message.size() + 1);
}

void ClientData::listen() {
  std::function<void(void)> fn = std::bind(&ClientData::connectionHandler, this);
  m_client_thread = std::thread(fn);
}

void ClientData::stopListening() {
  m_client_thread.join();
}

void ClientData::connectionHandler() {
  LogInfo << SERVER_PREFIX << "client connected";
  this->write(SERVER_PREFIX + "connected");

  // TODO: listen to incoming traffic
}
