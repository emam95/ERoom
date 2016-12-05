//
// Created by emam on 12/3/16.
//

#ifndef CHATSERVICE_SOCKHELPERS_H
#define CHATSERVICE_SOCKHELPERS_H


#include "helperheaders.h"

int initServer();

void terminateServer(int sockfd);

int rsend(int sockfd, sockaddr_storage their_addr, socklen_t addr_len, const packet p);

void thSend(int sockfd, sockaddr_storage their_addr, socklen_t addr_len);

void thRecieve(int sockfd, sockaddr_storage their_addr, socklen_t addr_len);

void calcCheckSum(packet p);

bool checksumValid(packet p);

#endif //CHATSERVICE_SOCKHELPERS_H
