//
// Created by emam on 12/3/16.
//

#ifndef CHATSERVICE_SOCKHELPERS_H
#define CHATSERVICE_SOCKHELPERS_H


#include <netdb.h>
#include "helperheaders.h"

int rsend(int sockfd, sockaddr_storage their_addr, socklen_t addr_len, const packet p);

void thRecieve(int sockfd);

void parseAt(const char* buffer, int size, std::vector<std::string>& output, char c);

void calcCheckSum(packet p);

bool checksumValid(packet p);

int readFile(const std::string* name, char* buffer);

void writeFile(const std::string* name, const char* buffer);

void serialize(const char* buffer, int size, std::vector<packet>& spackets);

void deserialize(const std::vector<packet>& spackets, char* buffer);

int gbnsend(const std::vector<packet>& spackets, int windowSize, int sockfd, addrinfo* ai);

void gbnrecieve(std::vector<packet> &sp, int sockfd, addrinfo* ai);

void sendFile(const std::string &fileName,sockaddr_storage their_addr);

void receiveFile(const std::string &fileName, sockaddr_storage their_addr);

int min(int x, int y);

#endif //CHATSERVICE_SOCKHELPERS_H
