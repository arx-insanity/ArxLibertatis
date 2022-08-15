#ifndef ARX_NETWORK_SERVER_H
#define ARX_NETWORK_SERVER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

#include "core/GameTime.h"

void *startServer(void *);

void stopServer();

void __connect(int clientId);

void __disconnect(int clientId);

void __broadcast(int sender, char *message);

void *connection_handler(void *clientSocketDescriptor);

#endif // ARX_NETWORK_SERVER_H
