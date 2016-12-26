//
// Created by emam on 12/3/16.
//

#ifndef CHATSERVICE_SOCKHELPERS_H
#define CHATSERVICE_SOCKHELPERS_H


#include "helperheaders.h"

int initServer(const char* name, const char* port);

int createSocket(const char* addr, const char* port, struct addrinfo **outai);

void terminateServer(int sockfd);

int rsend(int sockfd, addrinfo* ai, packet p);

void clientRecieve( entity e, std::queue<std::string> &mq, std::vector<entity> &entities );

void broadcast(std::queue<std::string> &mq, std::vector<entity> &entities);

int readFile(const std::string* name, char* buffer);

void writeFile(const std::string* name, const char* buffer);

void serialize(const char* buffer, int size, std::vector<packet>& spackets);

void deserialize(const std::vector<packet>& spackets, char* buffer);

int gbnsend(const std::vector<packet>& spackets, int windowSize, int sockfd, addrinfo* ai);

int gbnsend(const std::vector<packet>& spackets, int windowSize, int sockfd, sockaddr_storage their_addr);

void gbnrecieve(std::vector<packet> &sp, int sockfd);

void sendFile(const std::string &fileName,sockaddr_storage their_addr);

void receiveFile(const std::string &fileName, sockaddr_storage their_addr);

int min(int x, int y);

void parseAt(const char* buffer, int size, std::vector<std::string>& output, char c);

void parseAt(const char* buffer, int size, std::vector<std::string>& output, char c, bool ignorespace);

void calcCheckSum(packet& p);

bool checksumValid(packet p);

#endif //CHATSERVICE_SOCKHELPERS_H
