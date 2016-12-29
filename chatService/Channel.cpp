//
// Created by emam on 12/29/16.
//

#include <thread>
#include "Channel.h"
#include "sockhelpers.h"

using namespace std;

Channel::Channel()
{
    is_public = true;
}

Channel::Channel(int i)
{
    is_public = true;
    id = i;
}

Channel::~Channel()
{

}

Channel::Channel(string password)
{
    is_public = false;
    pass = password;
}

void Channel::AddClient(entity e)
{
    thread thread(clientRecieve, e, std::ref(mq), std::ref(entities));
    thread.detach();
}

bool Channel::ValidateEntry(string password)
{
    if(is_public)
        return true;

    return (password == pass);

}

void Channel::Run()
{
    is_running = true;
    cout << "channel " << id << " is active" << endl;
    thread bthread(broadcast, std::ref(mq), std::ref(entities));
    bthread.detach();
}