//
// Created by emam on 12/3/16.
//

#ifndef CHATSERVICE_SOCKHELPERS_H
#define CHATSERVICE_SOCKHELPERS_H


#include "helperheaders.h"

int initServer();

void terminateServer(int sockfd);

int rsend(int sockfd, addrinfo* ai, packet p);

void clientRecieve( entity e, std::queue<std::string> &mq, std::vector<entity> &entities );

void broadcast(std::queue<std::string> &mq, std::vector<entity> &entities);

int readFile(const std::string* name, char* buffer);

void writeFile(const std::string* name, const char* buffer);

void serialize(const char* buffer, int size, std::vector<packet>& spackets);

void deserialize(const std::vector<packet>* spackets, std::string* buffer);

void gbnsend();

void gbnrecieve();

void sendFile();

void receiveFile();

//void thSend(int id, const std::string addr, std::string port, std::vector<entity> &entities);

//void thRecieve(int id, std::vector<entity> &entities);

//void calcCheckSum(packet p);

//bool checksumValid(packet p);

#endif //CHATSERVICE_SOCKHELPERS_H
