//
// Created by emam on 12/3/16.
//

#include <iostream>
#include <vector>
#include <sys/socket.h>
#include <string.h>
#include <mutex>
#include <fstream>
#include <unistd.h>

#include "sockhelpers.h"

#define ISspace(x) isspace((int)(x))

int initServer(const char* name, const char* port)
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

    std::cout << "Server: waiting for connections..." << std::endl;

    return sockfd;

}

void terminateServer(int sockfd)
{
    close(sockfd);
}

int createSocket(const char* addr, const char* port, struct addrinfo **outai)
{
    struct addrinfo hints, *servinfo, *ai;
    int err;
    int sockfd;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((err = getaddrinfo(addr, port, &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return -1;
    }

    // loop through all the results and make a socket
    for(ai = servinfo; ai != NULL; ai = ai->ai_next)
    {
        if ((sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1)
        {
            perror("RThread: socket");
            continue;
        }

        break;
    }

    if (ai == NULL)
    {
        fprintf(stderr, "RThread: failed to create socket\n");
        return -1;
    }

    *outai = ai;

    freeaddrinfo(servinfo);

    return sockfd;

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

int rsend(int sockfd, addrinfo* ai, const packet p)
{
    while(1)
    {
        if ((sendto(sockfd, &p, sizeof(p), 0, ai->ai_addr, ai->ai_addrlen)) == -1)
        {
            perror("Server: sendto");
            return -1;
        }

        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set); // clear the set
        FD_SET(sockfd, &set); // add our file descriptor to the set
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
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
    int n = 0;

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
        sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&their_addr, addr_len);

        if(strcmp(pack.data, "") && n != 1)
        {
            std::cout << pack.data << std::endl;
            strcpy(pack.data, "");
        }
        n++;

    }
}

void parseAt(const char* buffer, int size, std::vector<std::string>& output, char c)
{
    for(int i = 0; i < size; i++)
    {
        std::string s("");
        while(!ISspace(buffer[i]) && buffer[i] != c && i < size)
        {
            s.push_back(buffer[i]);
            i++;
        }
        if(strcmp(s.c_str(), ""))
        {
            output.push_back(s);
        }

    }
}

int readFile(const std::string* name, char* buffer)
{

    std::streampos size;
    char* memblock;
    int len;

    //std::ifstream file ("example.bin", ios::in|ios::binary|ios::ate);
    std::ifstream file (name->c_str(), std::ios::in|std::ios::ate);
    if (file.is_open())
    {
        size = file.tellg();
        memblock = new char [size];
        len = size;
        file.seekg (0, std::ios::beg);
        file.read (memblock, size);
        file.close();

        memcpy(buffer, memblock, size);

        delete[] memblock;
    }
    else std::cout << "Unable to open file" << std::endl;

    return len;

}

void writeFile(const std::string* name, const char* buffer)
{
    std::ofstream myfile (name->c_str());
    if (myfile.is_open())
    {
        myfile << buffer;
        myfile.close();
    }
    else std::cout << "Unable to open file" << std::endl;
}

/*
void serialize(const char* buffer, int size, std::vector<packet>& spackets)
{
    packet lenpack;
    sprintf(lenpack.data, "$%d", (int)(size/500.0+0.5));
    spackets.push_back(lenpack);
    for(int i = 0; i < size; i++)
    {
        packet p;
        for(int j = 0; j < 500; j++)
        {
            if(i < size)
            {
                p.data[j] = buffer[i];
                i++;
            }
            else
                break;
        }
        spackets.push_back(p);
    }
}
 */

void serialize(const char* buffer, int size, std::vector<packet>& spackets)
{
    packet lenpack;
    sprintf(lenpack.data, "$%d%c", (int)(size/500.0+0.5), NULL);
    spackets.push_back(lenpack);
    for(int i = 0; i < size; i++)
    {
        packet p;
        bool packet_complete = true;
        for(int j = 0; j < 500; j++)
        {
            if(i < size)
            {
                p.data[j] = buffer[i];
                i++;
            }
            else{
                p.data[j] = NULL;
                packet_complete = false;
            }
        }
        if(packet_complete){
            p.data[500] = NULL;
            i--;
        }
        spackets.push_back(p);
    }
}

void deserialize(const std::vector<packet>& spackets, char* buffer)
{
    int buffer_index = 0;
    for(int i = 1 ; i < spackets.size() ; i++)
    {
        for(int j = 0 ; j < strlen(spackets[i].data) ; j++)
        {
            buffer[buffer_index++] = spackets[i].data[j];
        }
    }
    buffer[buffer_index] = NULL;
}

int gbnsend(const std::vector<packet>& spackets, int windowSize, int sockfd, sockaddr_storage their_addr)
{
    int npackets = spackets.size();
    int base = 0;
    int sentNotAck = base;
    int N = windowSize - 1;

    while((sentNotAck < N) && (N < npackets))
    {
        packet p = spackets[sentNotAck];
        p.seqno = sentNotAck;
        if ((sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1)
        {
            perror("GBN: sendto");
            return -1;
        }
        sentNotAck++;
    }

    while(base < npackets-1)
    {
        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set); // clear the set
        FD_SET(sockfd, &set); // add our file descriptor to the set
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int rv = select(sockfd + 1, &set, NULL, NULL, &timeout);
        if (rv == -1)
        {
            perror("socket error");
            return -1;
        }
        else if (rv == 0)
        {
            // timeout, socket does not have anything to read
            std::cout << "time out occured: Resending window" << std::endl;
            sentNotAck = base;
            while(sentNotAck < N)
            {
                packet p = spackets[sentNotAck];
                p.seqno = sentNotAck;
                if ((sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1)
                {
                    perror("GBN: sendto");
                    return -1;
                }
                sentNotAck++;
            }

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
            if(ack.ackno > base)
            {
                int delta = ack.ackno - base;
                base += delta;
                N = min(N + delta, npackets);
                while(sentNotAck < N)
                {
                    packet p = spackets[sentNotAck];
                    p.seqno = sentNotAck;
                    if ((sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1)
                    {
                        perror("GBN: sendto");
                        return -1;
                    }
                    sentNotAck++;
                }

            }

            continue;
        }
    }

}

void gbnrecieve(std::vector<packet> &sp, int sockfd, addrinfo* ai)
{
    int esqn = 0;
    int numbytes;
    int npackets;
    int received = 0;
    packet lenpack;

    if ((numbytes = recvfrom(sockfd, &lenpack, sizeof(lenpack) , 0, nullptr, nullptr)) == -1)
    {
        perror("recvfrom");
        exit(1);
    }
    std::vector<std::string> output;
    parseAt(lenpack.data, sizeof(lenpack.data), output, '$');

    for(int i = 0; i < output.size(); i++)
        std::cout << output[i] << std::endl;

    npackets = std::stoi(output[0]);

    while(received < npackets)
    {
        packet pack;
        if ((numbytes = recvfrom(sockfd, &pack, sizeof(pack) , 0, nullptr, nullptr)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        ack_packet ack;
        ack.ackno = esqn;

        if(esqn == pack.seqno)
        {
            sendto(sockfd, &ack, sizeof(ack), 0, ai->ai_addr, ai->ai_addrlen );
            sp.push_back(pack);
            esqn++;
            received++;
        }

    }
}

int gbnsend(const std::vector<packet>& spackets, int windowSize, int sockfd, addrinfo* ai)
{
    int npackets = spackets.size();
    int base = 0;
    int sentNotAck = base;
    int N = windowSize - 1;

    while((sentNotAck < N) && (N < npackets))
    {
        packet p = spackets[sentNotAck];
        if ((sendto(sockfd, &p, sizeof(p), 0, ai->ai_addr, ai->ai_addrlen)) == -1)
        {
            perror("GBN: sendto");
            return -1;
        }
        sentNotAck++;
    }

    while(base < npackets)
    {
        fd_set set;
        struct timeval timeout;
        FD_ZERO(&set); // clear the set
        FD_SET(sockfd, &set); // add our file descriptor to the set
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        int rv = select(sockfd + 1, &set, NULL, NULL, &timeout);
        if (rv == -1)
        {
            perror("socket error");
            return -1;
        }
        else if (rv == 0)
        {
            // timeout, socket does not have anything to read
            std::cout << "time out occured: Resending window" << std::endl;
            sentNotAck = base;
            while(sentNotAck < N)
            {
                packet p = spackets[sentNotAck];
                if ((sendto(sockfd, &p, sizeof(p), 0, ai->ai_addr, ai->ai_addrlen)) == -1)
                {
                    perror("GBN: sendto");
                    return -1;
                }
                sentNotAck++;
            }

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
            if(ack.ackno > base)
            {
                int delta = ack.ackno - base;
                base += delta;
                N = min(N + delta, npackets);
                while(sentNotAck < N)
                {
                    packet p = spackets[sentNotAck];
                    if ((sendto(sockfd, &p, sizeof(p), 0, ai->ai_addr, ai->ai_addrlen)) == -1)
                    {
                        perror("GBN: sendto");
                        return -1;
                    }
                    sentNotAck++;
                }

            }

            continue;
        }
    }

}

void sendFile(const std::string &fileName,sockaddr_storage their_addr)
{
    char s[200 * 1024];
    std::vector<packet> sp;
    int size;
    int err;

    size = readFile(&fileName, s);
    serialize(s, size, sp);

    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
    {
        perror("FILE: socket");
    }

    packet pack;
    pack.seqno = 0;
    std::string s2("~u~" + fileName);
    strcpy(pack.data, s2.c_str());

    if ((sendto(sockfd, &pack, sizeof(pack), 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1)
    {
        perror("FILE: sendto");
        return;
    }

    int numbytes;
    packet pack2;
    ack_packet ack;

    struct sockaddr_storage their_addr2;

    socklen_t addr_len;

    addr_len = sizeof their_addr2;

    if ((numbytes = recvfrom(sockfd, &pack2, sizeof(pack2) , 0, (struct sockaddr *)&their_addr2, &addr_len) == -1))
    {
        perror("recvfrom");
        exit(1);
    }

    ack.ackno = pack2.seqno;

    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&their_addr2, sizeof(their_addr2) );

    gbnsend(sp, 5, sockfd, their_addr2);

    close(sockfd);


}

void receiveFile(const std::string &fileName, sockaddr_storage their_addr)
{
    struct addrinfo *ai;
    int sockfd;
    int err;
    char ip[INET6_ADDRSTRLEN];
    char port[6];

    err = getnameinfo(((struct sockaddr*)&their_addr), sizeof(their_addr), ip, sizeof(ip), port, sizeof (port), NI_NUMERICHOST | NI_NUMERICSERV);

    std::string ipstr(ip);
    std::string portstr(port);


    packet pack;
    pack.seqno = 0;
    sockfd =  createSocket(ipstr.c_str(), portstr.c_str(), &ai);

    if ((rsend(sockfd, ai, pack)) == -1)
    {
        perror("RThread: sendto");
        return;
    }

    char s[200 * 1024];
    std::vector<packet> sp;

    gbnrecieve(sp, sockfd, ai);

    close(sockfd);

    deserialize(sp, s);

    writeFile(&fileName, s);
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

int min(int x, int y)
{
    if(x<y)
        return x;
    return y;
}