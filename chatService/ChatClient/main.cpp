#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <iostream>
#include <thread>

#include "helperheaders.h"
#include "sockhelpers.h"


int main()
{
    int sockfd;
    struct addrinfo hints, *servinfo, *p;
    int rv;
    int numbytes;
    char msg[100];
    //std::string input;

    std::cout << "Enter Your Name: " << std::endl;
    std::cin.getline(msg, sizeof(msg));

    //strcpy(msg, input.c_str());

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((rv = getaddrinfo("localhost", MAINPORT, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and make a socket
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype,
                             p->ai_protocol)) == -1) {
            perror("talker: socket");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "talker: failed to create socket\n");
        return 2;
    }

    if ((numbytes = sendto(sockfd, msg, strlen(msg), 0, p->ai_addr, p->ai_addrlen)) == -1) {
        perror("talker: sendto");
        exit(1);
    }

    freeaddrinfo(servinfo);

    struct sockaddr_storage their_addr;

    packet pack;
    ack_packet ack;
    socklen_t addr_len;

    addr_len = sizeof their_addr;

    if ((numbytes = recvfrom(sockfd, &pack, sizeof(pack) , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }

    //cstd::cout << pack.data << std::endl;

    ack.ackno = pack.seqno;
    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&their_addr, addr_len);

    std::thread thread(thRecieve, sockfd);
    thread.detach();

    packet pack2;
    pack2.seqno = 0;

    while(1)
    {
        //input.clear();
        std::cin.clear();

        std::cin.getline(msg,sizeof(msg));

        strcpy(pack2.data, msg);

        if ((rsend(sockfd, their_addr, addr_len, pack2)) == -1) {
            perror("Server: sendto");
            return 1;
        }

        pack2.seqno = (pack2.seqno + 1) % 2;
    }

    /*
    while(1)
    {
        if ((numbytes = recvfrom(sockfd, &pack, sizeof(pack) , 0, nullptr, nullptr) == -1))
        {
            perror("recvfrom");
            exit(1);
        }

        ack.ackno = pack.seqno;

        sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&their_addr, addr_len);

        std::cout << pack.data << std::endl;
    }*/

    return 0;
}