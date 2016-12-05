//
// Created by emam on 12/3/16.
//

#ifndef CHATSERVICE_SOCKHELPERS_H
#define CHATSERVICE_SOCKHELPERS_H


#include "helperheaders.h"

int initServer();

void terminateServer(int sockfd);

int rsend(int sockfd, addrinfo* ai, packet p);

void thSend(int id, const std::string addr, std::string port, std::vector<entity> &entities);

void thRecieve(int id, std::vector<entity> &entities);

void calcCheckSum(packet p);

bool checksumValid(packet p);

#endif //CHATSERVICE_SOCKHELPERS_H
