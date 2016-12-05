//
// Created by emam on 12/3/16.
//

#ifndef CHATSERVICE_HELPERHEADERS_H
#define CHATSERVICE_HELPERHEADERS_H

#include <stdint-gcc.h>

#define MAINPORT "5000"
#define MAXBUFLEN 1024

struct packet
{
/* Header */
    uint16_t cksum;
    uint16_t len;
    uint32_t seqno;
/* Data */
    char data[500];
};

struct ack_packet
{
    uint16_t cksum;
    uint16_t len;
    uint32_t ackno;
};

struct entity
{
    int sockfd;
    std::string name;
    int lastSent;
    int lastRecieved;
    char buffer[450];
};

#endif //CHATSERVICE_HELPERHEADERS_H
