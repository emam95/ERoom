//
// Created by emam on 12/29/16.
//

#include "Server.h"
#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <cstring>
#include <queue>

#include "sockhelpers.h"

using namespace std;

Server::Server(const char* name, const char* port)
{
    lis_sockfd = initServer(name, port);
    channels.reserve(5);
    for(int i = 0; i < 5; i++)
    {
        Channel ch(i);
        channels.push_back(ch);
    }
}

int Server::initServer(const char *name, const char *port)
{
    struct addrinfo hints, *servinfo, *p;
    int err;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((err = getaddrinfo(name, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        exit(1);
    }

    // loop through all the results and bind to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
        {
            perror("listener: socket");
            continue;
        }

        if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1)
        {
            close(sockfd);
            perror("listener: bind");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "listener: failed to bind socket\n");
        exit(1);
    }

    freeaddrinfo(servinfo);

    cout << "Server: waiting for connections..." << std::endl;

    return sockfd;

}

Server::~Server()
{
    close(lis_sockfd);
}

void Server::Run()
{
    int id = 0;
    while(1)
    {
        struct sockaddr_storage their_addr;
        int numbytes;

        char buf[MAXBUFLEN];
        socklen_t addr_len;

        addr_len = sizeof their_addr;
        if ((numbytes = recvfrom(lis_sockfd, buf, MAXBUFLEN-1 , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        char ip[INET6_ADDRSTRLEN];
        char port[6];

        int err = getnameinfo(((struct sockaddr*)&their_addr), sizeof(their_addr), ip, sizeof(ip), port, sizeof (port), NI_NUMERICHOST | NI_NUMERICSERV);

        std::string ipstr(ip);
        std::string portstr(port);

        std::cout << "Server connected to ip " << ipstr << " port " << portstr << " id no " << id << std::endl;

        entity e;
        e.addr = ipstr;
        e.port = portstr;
        e.name = buf;
        e.id = id;
        e.cid = 0;

        if(!channels[0].is_running)
            channels[0].Run();

        channels[0].AddClient(e);

        id++;
    }
}