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
#include <queue>
#include <fstream>
#include "sockhelpers.h"

#define ISspace(x) isspace((int)(x))

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

void clientRecieve( entity e, std::queue<std::string> &mq, std::vector<entity> &entities )
{
    struct addrinfo hints, *servinfo, *ai;
    int err;
    int sockfd;
    std::string addr = e.addr;
    std::string port = e.port;
    int id = e.id;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;

    if ((err = getaddrinfo(addr.c_str(), port.c_str(), &hints, &servinfo)) != 0)
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return;
    }

    // loop through all the results and make a socket
    for(ai = servinfo; ai != NULL; ai = ai->ai_next)
    {
        if ((sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) == -1) {
            perror("RThread: socket");
            continue;
        }

        break;
    }

    if (ai == NULL)
    {
        fprintf(stderr, "RThread: failed to create socket\n");
        return;
    }

    packet pack;
    pack.seqno = 0;
    sprintf(pack.data, "id: %d", id);
    e.sockfd = sockfd;
    e.ai = ai;

    mu.lock();
    entities.push_back(e);
    mq.push(e.name + " has entered chat");
    mu.unlock();

    if ((rsend(sockfd, ai, pack)) == -1)
    {
        perror("RThread: sendto");
        return;
    }

    freeaddrinfo(servinfo);

    int numbytes;
    ack_packet ack;
    packet pack2;

    while(1)
    {
        if ((numbytes = recvfrom(sockfd, &pack2, sizeof(pack2) , 0, nullptr, nullptr)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        ack.ackno = pack2.seqno;

        sendto(sockfd, &ack, sizeof(ack), 0, ai->ai_addr, ai->ai_addrlen );

        if(strcmp(pack2.data, ""))
        {
            mq.push(e.name + " : " + std::string(pack2.data));
            std::cout << pack2.data << std::endl;
            strcpy(pack2.data, "");
        }

    }

}

void broadcast(std::queue<std::string> &mq, std::vector<entity> &entities)
{
    packet pack;
    pack.seqno = 1;
    while(1)
    {
        if(!mq.empty())
        {
            std::string message;
            message = mq.front();
            strcpy(pack.data, message.c_str());
            pack.seqno = (pack.seqno + 1) % 2;
            pack.len = sizeof(pack.data);
            //calcCheckSum(pack);
            for(int i = 0; i < entities.size(); i++)
            {
                if ((rsend(entities[i].sockfd, entities[i].ai, pack)) == -1)
                {
                    perror("Broadcast: sendto");
                    return;
                }
            }
            mq.pop();
        }
        usleep(1000);
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

void deserialize(const std::vector<packet>& spackets, char* buffer)
{

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

void sendFile()
{
    char s[200 * 1024];
    std::string name1("uniform_buffer_object.txt");
    std::vector<packet> sp;
    int size;

    size = readFile(&name1, s);
    serialize(s, size, sp);
}

void receiveFile()
{
    char s[200 * 1024];
    std::string name2("output.txt");
    std::vector<packet> sp;
    deserialize(sp, s);

    writeFile(&name2, s);
}

int min(int x, int y)
{
    if(x<y)
        return x;
    return y;
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



void calcCheckSum(packet& p)
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