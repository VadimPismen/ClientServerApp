#pragma once

#include "../common.h"
#include "ClientThread.h"

namespace CSA
{
    class ClientThread;
    
    class ServerClass
    {
    public:
        ServerClass(uint16_t port);
        ~ServerClass();

        void OpenServer();
        void DeleteClient(int socket);

    private:
        
        uint16_t port_;
        int serverSocket_;
        struct sockaddr_in addr_;

        boost::thread RequestsServitor_;
        std::map<int, ClientThread> listOfClients_;
    };
}