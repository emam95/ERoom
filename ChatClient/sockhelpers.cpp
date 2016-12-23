//
// Created by emam on 12/3/16.
//

#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <string.h>
#include <mutex>

#include "sockhelpers.h"

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

void thRecieve(int sockfd)
{
    int numbytes;
    packet pack;
    ack_packet ack;
    sockaddr_storage their_addr;
    socklen_t addr_len;

    while(1)
    {
        if ((numbytes = recvfrom(sockfd, &pack, sizeof(pack) , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        ack.ackno = pack.seqno;

        //double r = ((double) rand() / (RAND_MAX));

        //if(r < 0.9 && checksumValid(pack))
        //if(r < 0.9)
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