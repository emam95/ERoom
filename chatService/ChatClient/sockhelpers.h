//
// Created by emam on 12/3/16.
//

#ifndef CHATSERVICE_SOCKHELPERS_H
#define CHATSERVICE_SOCKHELPERS_H


#include "helperheaders.h"

int rsend(int sockfd, sockaddr_storage their_addr, socklen_t addr_len, const packet p);

void thRecieve(int sockfd);

void calcCheckSum(packet p);

bool checksumValid(packet p);

#endif //CHATSERVICE_SOCKHELPERS_H
