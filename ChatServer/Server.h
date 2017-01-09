//
// Created by emam on 12/29/16.
//

#ifndef CHATSERVICE_SERVER_H
#define CHATSERVICE_SERVER_H

#include "Channel.h"

class Server
{
    public:
        int lis_sockfd;

        Server() : Server(NULL, MAINPORT){};
        Server(const char* name, const char* port);
        ~Server();

        void Run();

    private:
        int initServer(const char* name, const char* port);
        std::vector<Channel> channels;

};


#endif //CHATSERVICE_SERVER_H
