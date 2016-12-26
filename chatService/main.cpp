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

#include "helperheaders.h"
#include "sockhelpers.h"

int main()
{
    int lis_sockfd = initServer(NULL, MAINPORT);
    int id = 0;
    std::vector<entity> entities;
    entities.reserve(10);
    std::queue<std::string> mq;

    std::thread bthread(broadcast, std::ref(mq), std::ref(entities));
    bthread.detach();

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

        std::thread thread(clientRecieve, e, std::ref(mq), std::ref(entities));
        thread.detach();

        id++;
    }

    //we can set a condition to break the loop and terminate server
    terminateServer(lis_sockfd);
    return 0;
}