#pragma once

#include "../common.h"
#include "ClientThread.h"

namespace CSA
{
    class ClientThread;
    
    class ServerClass
    {
    public:
        ServerClass(std::string cfgFile);
        ~ServerClass();

        void OpenServer();
        void DeleteClient(int socket);

        bool LookForAccount(std::string login, std::string password);
        std::string GetUserDirFromConfs(std::string login);
        void SaveUserDir(std::string login, std::string dir);
        

    private:
        
        std::string cfgFile_;
        libconfig::Config cfg_;

        int port_;
        int serverSocket_;
        struct sockaddr_in addr_;

        std::map<int, ClientThread> listOfClients_;
    };
}