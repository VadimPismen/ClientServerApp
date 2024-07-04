#pragma once

#include "../common.h"
#include "ClientThread.h"

namespace CSA
{
    class ClientThread;
    
    class ServerClass
    {
    public:
        ServerClass(uint16_t port, std::string cfgFile);
        ~ServerClass();

        void OpenServer();
        void DeleteClient(int socket);

        bool LookForAccount(std::string login, std::string password);
        

    private:
        
        std::string cfgFile_;

        uint16_t port_;
        int serverSocket_;
        struct sockaddr_in addr_;

        boost::thread RequestsServitor_;
        std::map<int, ClientThread> listOfClients_;
    };
}