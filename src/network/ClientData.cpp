#include <sys/socket.h>
#include <boost/algorithm/string/trim.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/erase.hpp>
#include "io/log/Logger.h"
#include "network/common.h"
#include "network/ClientData.h"

ClientData::ClientData(int descriptor, Server * server) {
  this->m_descriptor = descriptor;
  this->m_nickname = "client #" + std::to_string(descriptor);
  this->m_entity = nullptr;
  this->m_server = server;
}

int ClientData::getDescriptor() {
  return this->m_descriptor;
}

std::string ClientData::getNickname() {
  return this->m_nickname;
}

void ClientData::write(std::string message) {
  ::write(this->m_descriptor, (message + EOL).c_str(), message.size() + 1);
}

std::string ClientData::read() {
  char rawInput[2000];
  int rawReadSize = recv(this->m_descriptor, rawInput, 2000, 0);

  if (rawReadSize == 0) {
    return "/quit";
  }

  // TODO: error handling
  if (rawReadSize == -1) {
    // failed to read with recv
  }

  std::string input(rawInput, rawReadSize);
  boost::trim(input);

  return input;
}

void ClientData::listen() {
  this->m_thread = new std::thread(&ClientData::connectionHandler, this);
}

void ClientData::stopListening() {
  this->m_thread->join();
}

void ClientData::connectionHandler() {
  LogInfo << SERVER_PREFIX << "client #" << std::to_string(this->m_descriptor) << " connected";
  this->write(SERVER_PREFIX + "connected, welcome to Arx!");

  this->m_server->broadcast(this, "joined");

  bool clientWantsToQuit = false;

  do {
    std::string input = this->read();

    if (!input.empty()) {
      if (boost::starts_with(input, "/")) {
        std::string::size_type commandSize = input.find(" ", 0);
        std::string command = input.substr(1, commandSize - 1);
        std::string args = boost::trim_copy(boost::erase_head_copy(input, commandSize));

        if (command == "exit" || command == "quit") {
          this->write(SERVER_PREFIX + "disconnected, goodbye!");
          clientWantsToQuit = true;
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
  } while (!clientWantsToQuit);
  
  if (clientWantsToQuit) {
    fflush(stdout);
  }

  LogInfo << SERVER_PREFIX << "client #" << std::to_string(this->m_descriptor) << " disconnected";
  this->m_server->disconnect(this);
}
