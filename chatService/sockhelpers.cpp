//
// Created by emam on 12/3/16.
//

#include "sockhelpers.h"

#define ISspace(x) isspace((int)(x))

std::mutex mu;

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
            //std::cout << "time out occured" << std::endl;
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
    struct addrinfo *ai;
    int sockfd;
    int err;
    std::string addr = e.addr;
    std::string port = e.port;
    int id = e.id;

    packet pack;
    pack.seqno = 0;
    sprintf(pack.data, "id: %d", id);
    sockfd =  createSocket(addr.c_str(), port.c_str(), &ai);
    e.sockfd = sockfd;
    e.ai = ai;

    if ((rsend(sockfd, ai, pack)) == -1)
    {
        perror("RThread: sendto");
        return;
    }

    mu.lock();
    mq.push(e.name + " has entered the channel");
    mu.unlock();

    sprintf(pack.data, "You entered channel %d", e.cid);

    if ((rsend(sockfd, ai, pack)) == -1)
    {
        perror("RThread: sendto");
        return;
    }

    usleep(1000);

    mu.lock();
    entities.push_back(e);
    mu.unlock();

    int numbytes;
    ack_packet ack;
    packet pack2;

    while(1)
    {
        struct sockaddr_storage their_addr;
        int numbytes;
        socklen_t addr_len;
        addr_len = sizeof their_addr;

        if ((numbytes = recvfrom(sockfd, &pack2, sizeof(pack2) , 0, (struct sockaddr *)&their_addr, &addr_len)) == -1)
        {
            perror("recvfrom");
            exit(1);
        }

        ack.ackno = pack2.seqno;

        sendto(sockfd, &ack, sizeof(ack), 0, ai->ai_addr, ai->ai_addrlen );

        if(pack2.data[0] == '~')
        {
            std::vector<std::string> output;
            if(pack2.data[1] == 'u')
            {
                parseAt(pack2.data, sizeof(pack2.data), output, '~');
                receiveFile(output[1], their_addr);
                //std::thread th(receiveFile,output[1], their_addr);
                //th.detach();
                mq.push(e.name + " uploaded file " + output[1]);
            }
            else if(pack2.data[2] == 'd')
            {

            }
            continue;
        }

        if(strcmp(pack2.data, ""))
        {
            mq.push(e.name + " : " + std::string(pack2.data));
            //std::cout << pack2.data << std::endl;
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
            calcCheckSum(pack);
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
    for(int i = 0 ; i < spackets.size() ; i++)
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
        if ((sendto(sockfd, &p, sizeof(p), 0, (struct sockaddr *)&their_addr, sizeof(their_addr))) == -1)
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
        timeout.tv_sec = 5;
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

void gbnrecieve(std::vector<packet> &sp, int sockfd)
{
    int esqn = 0;
    int numbytes;
    int npackets;
    int received = 0;
    packet lenpack;

    struct sockaddr_storage their_addr;

    socklen_t addr_len;

    addr_len = sizeof their_addr;

    if ((numbytes = recvfrom(sockfd, &lenpack, sizeof(lenpack) , 0, (struct sockaddr *)&their_addr, &addr_len) == -1))
    {
        perror("recvfrom");
        exit(1);
    }
    std::vector<std::string> output;
    parseAt(lenpack.data, sizeof(lenpack.data), output, '$');

    //for(int i = 0; i < output.size(); i++)
        //std::cout << output[i] << std::endl;

    npackets = std::stoi(output[0]);

    esqn++;

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
            sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&their_addr, addr_len );
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

    int sockfd = socket(AF_UNSPEC, SOCK_DGRAM, 0);
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

    if ((numbytes = recvfrom(sockfd, &pack2, sizeof(pack2) , 0, nullptr, nullptr) == -1))
    {
        perror("recvfrom");
        exit(1);
    }

    ack.ackno = pack2.seqno;

    sendto(sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&their_addr, sizeof(their_addr) );

    gbnsend(sp, 5, sockfd, their_addr);

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

    /*
    if ((sendto(sockfd, &pack, sizeof(pack), 0, ai->ai_addr, ai->ai_addrlen)) == -1)
    {
        perror("GBN: sendto");
        return;
    }*/

    char s[200 * 1024];
    std::vector<packet> sp;

    gbnrecieve(sp, sockfd);

    close(sockfd);

    deserialize(sp, s);

    writeFile(&fileName, s);
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

void parseAt(const char* buffer, int size, std::vector<std::string>& output, char c, bool ignorespace)
{
    for(int i = 0; i < size; i++)
    {
        if(ignorespace && ISspace(buffer[i]))
            continue;

        std::string s("");
        while(buffer[i] != c && i < size)
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