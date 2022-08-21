#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "core/Core.h"
#include "game/Player.h"
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
  MessageType messageType;
  ::read(this->m_socketDescriptor, (char *)&messageType, sizeof(messageType));

  if (messageType == MessageTypeServerStopped) {
    return "/quit";
  }
  
  if (messageType == MessageTypeChangeLevel) {
    char rawInput[10];
    int rawReadSize = ::read(this->m_socketDescriptor, rawInput, 10);
    std::string input(rawInput, rawReadSize);

    return "/changeLevelTo " + input;
  }

  return "";
}

void Client::write(std::string message) {
  // ::write(this->m_socketDescriptor, (message + EOL).c_str(), message.size() + 1);
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

  LogInfo << CLIENT_PREFIX << "connected";

  do {
    std::string input = this->read();
    if (input != "") {
      if (boost::starts_with(input, "/")) {
        std::string::size_type commandSize = input.find(" ", 0);
        std::string command = input.substr(1, commandSize - 1);
        std::string args = boost::trim_copy(boost::erase_head_copy(input, commandSize));

        LogInfo << CLIENT_PREFIX << "/" + command + " " + args;

        if (command == "quit" || command == "exit") {
          this->m_isQuitting = true;
        } else if (command == "changeLevelTo") {
          long int level = strtol(args.c_str(), nullptr, 10);
          this->changeLevel(level);
        }
      }
    }
  } while (!this->m_isQuitting);

  if (this->m_isQuitting) {
    fflush(stdout);
  }
}

void Client::changeLevel(long int level) {
  TELEPORT_TO_LEVEL = level;
  TELEPORT_TO_POSITION = "";
  TELEPORT_TO_ANGLE = static_cast<long>(player.angle.getYaw());
  CHANGE_LEVEL_ICON = ChangeLevelNow;
}
