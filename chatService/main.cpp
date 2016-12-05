#include <iostream>
#include <thread>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <vector>
#include <cstring>

#include "helperheaders.h"
#include "sockhelpers.h"

int main()
{
    int lis_sockfd = initServer();
    int id = 0;
    std::vector<entity> entities;
    entities.reserve(10);

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
        e.lastRecieved = 0;
        e.lastSent = -1;
        e.name = buf;
        strcpy(e.buffer, "has entered chat");
        entities.push_back(e);

        std::thread thread1(thSend, id, ipstr, portstr, std::ref(entities));
        thread1.detach();

        sleep(3);

        std::thread thread2(thRecieve, id, std::ref(entities));
        thread2.detach();
        id++;


    }

    //we can set a condition to break the loop and terminate server
    terminateServer(lis_sockfd);
    return 0;
}