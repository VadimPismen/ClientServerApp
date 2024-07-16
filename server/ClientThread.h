#pragma once

#include "../common.h"
#include "ServerClass.h"

namespace CSA
{
        
    class ServerClass;

    class ClientThread{

    public:

        ClientThread();
        ~ClientThread();

        void StartWorking(int socket, ServerClass* parent);
        std::string getIP() const { return IP_;};
        std::string getName() const { return name_;};

    private:
        void WorkWithClient_();
        void DisconnectClient_();
        void LostConnection_();

        void ParseCommand_(const std::string command);

        std::string ExecuteSystemCommandAndGetResult_(const std::string command);

        std::string GetAbsolutePath_(const std::string path);

        ClientState state_ = ClientState::LOGIN;
        ServerClass* parent_;
        int socket_;
        std::string name_;
        std::string clientCD_ = std::filesystem::current_path().string();
        bool canWrite_ = true;

        boost::thread workingThread_;
        uint16_t port_;
        std::string IP_;

    };
}