//
// Created by emam on 12/29/16.
//

#ifndef CHATSERVICE_CHANNEL_H
#define CHATSERVICE_CHANNEL_H

#include <iostream>
#include <string>
#include <queue>
#include <vector>
#include "helperheaders.h"


class Channel
{
    public:
        std::vector<entity> entities;
        std::queue<std::string> mq;
        bool is_public;
        bool is_running;

        Channel();
        Channel(int id);
        Channel(std::string password);
        ~Channel();

        void AddClient(entity e);
        bool ValidateEntry(std::string password = "");
        void Run();

    private:
        std::string pass;
        int id;

};


#endif //CHATSERVICE_CHANNEL_H
