//
// Created by emam on 12/3/16.
//

#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <mutex>

#include "sockhelpers.h"

std::mutex mu;

int initServer()
{
    struct addrinfo hints, *servinfo, *p;
    int err;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((err = getaddrinfo(NULL, MAINPORT, &hints, &servinfo)) != 0)
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

    std::cout << "Server: waiting for connections..." << std::endl;

    return sockfd;

}

void terminateServer(int sockfd)
{
    close(sockfd);
}

int rsend(int sockfd, sockaddr_storage their_addr, socklen_t addr_len, const packet p)
{
    while(1)
    {
        if ((sendto(sockfd, &p, sizeof(p), 0,(struct sockaddr *)&their_addr, addr_len)) == -1)
        {
            perror("Server: sendto");
            return -1;
        }

        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set); // clear the set
        FD_SET(sockfd, &set); // add our file descriptor to the set
        timeout.tv_sec = 1;
        timeout.tv_usec = 0; //half a sec
        int rv = select(sockfd + 1, &set, NULL, NULL, &timeout);
        if (rv == -1)
        {
            perror("socket error");
            return -1;
        }
        else if (rv == 0)
        {
            // timeout, socket does not have anything to read
            std::cout << "time out occured" << std::endl;
            continue;
        }
        else
        {
            ack_packet ack;
            // socket has something to read
            if ((recvfrom(sockfd, &ack, sizeof(ack) , 0, nullptr, nullptr) == -1))
            {
                perror("recvfrom");
                exit(1);
            }
            if(ack.ackno == p.seqno)
            {
                return 0;
                break;
            }

            continue;
        }
    }
    return 0;
}

void thSend(int sockfd,sockaddr_storage their_addr, socklen_t addr_len)
{
    packet pack;
    pack.seqno = 0;
    char msg[50];
    std::string input;
    while(1)
    {
        input.clear();
        std::cin.clear();

        std::cin >> input;

        strcpy(msg, input.c_str());

        strcpy(pack.data, msg);

        if ((rsend(sockfd, their_addr, addr_len, pack)) == -1)
        {
            perror("Server: sendto");
            return;
        }

        pack.seqno = (pack.seqno + 1) % 2;

    }
}

void thRecieve(int sockfd, sockaddr_storage their_addr, socklen_t addr_len)
{
    int numbytes;
    packet pack;
    ack_packet ack;

    while(1)
    {
        if ((numbytes = recvfrom(sockfd, &pack, sizeof(pack) , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        ack.ackno = pack.seqno;

        double r = ((double) rand() / (RAND_MAX));

        //if(r < 0.9 && checksumValid(pack))
        if(r < 0.9)
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&their_addr, addr_len);

        if(strcmp(pack.data, ""))
        {
            std::cout << pack.data << std::endl;
            strcpy(pack.data, "");
        }

    }
}


void calcCheckSum(packet p)
{
    int sum = 0;
    for(int i = 0; i < (p.len-1); ++i)
    {
        sum += p.data[i];
    }

    sum %= 256; //because 1 byte

    char ch = sum;

    //twos complement
    unsigned char twoscompl = ~ch + 1;

    p.cksum = twoscompl;
}

bool checksumValid(packet p)
{
    int sum = 0;
    for(int i = 0; i < (p.len-1); ++i)
    {
        sum += p.data[i];
    }

    sum %= 256;

    char ch = sum;

    //twos complement
    unsigned char twoscompl = ~ch + 1;

    return (p.cksum == twoscompl);
}