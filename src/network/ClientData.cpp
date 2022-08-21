#include <sys/socket.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/ClientData.h"

ClientData::ClientData(int descriptor, Server * server) {
  this->m_clientDescriptor = descriptor;
  this->m_nickname = "client #" + std::to_string(descriptor);
  this->m_entity = nullptr;
  this->m_server = server;
  this->m_isQuitting = false;
}

int ClientData::getDescriptor() {
  return this->m_clientDescriptor;
}

std::string ClientData::getNickname() {
  return this->m_nickname;
}

void ClientData::write(MessageType messageType, std::string payload) {
  ::write(this->m_clientDescriptor, (char *)&messageType, sizeof(messageType));

  if (payload != "") {
    ::write(this->m_clientDescriptor, payload.c_str(), payload.size());
  }
}

std::string ClientData::read() {
  /*
  char rawInput[2000];
  int rawReadSize = ::read(this->m_clientDescriptor, rawInput, 2000);

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
  */

  return "";
}

void ClientData::listen() {
  this->m_thread = new std::thread(&ClientData::connectionHandler, this);
}

void ClientData::stopListening() {
  this->m_isQuitting = true;
  this->m_thread->join();
}

void ClientData::connectionHandler() {
  /*
  LogInfo << SERVER_PREFIX << "client #" << std::to_string(this->m_clientDescriptor) << " connected";
  this->write(SERVER_PREFIX + "connected, welcome to Arx!");

  this->m_server->broadcast(this, "joined");

  do {
    std::string input = this->read();

    LogInfo << "--- ClientData: got message from client '" << input << "'";

    if (!input.empty()) {
      if (boost::starts_with(input, "/")) {
        std::string::size_type commandSize = input.find(" ", 0);
        std::string command = input.substr(1, commandSize - 1);
        std::string args = boost::trim_copy(boost::erase_head_copy(input, commandSize));

        if (command == "exit" || command == "quit") {
          this->write(SERVER_PREFIX + "disconnected, goodbye!");
          this->m_isQuitting = true;
        } else if (command == "say") {
          if (args != "") {
            this->m_server->broadcast(this, "say", args);
          }
        } else if (command == "make-host") {
          this->m_server->broadcast(this, "make-host", args);
        } else if (command == "nickname") {
          if (args != "") {
            this->m_nickname = args;
          }
        }
      }
    }
  } while (!this->m_isQuitting);
  
  if (this->m_isQuitting) {
    fflush(stdout);
  }

  LogInfo << SERVER_PREFIX << "client #" << std::to_string(this->m_clientDescriptor) << " disconnected";
  this->m_server->disconnect(this);
  */
}
