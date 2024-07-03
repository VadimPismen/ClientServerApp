#pragma once

#include "../common.h"
#include "ServerClass.h"

namespace CSA
{
    
class ServerClass;

class ClientThread{

public:

    ClientThread(int socket, ServerClass* parent);
    ~ClientThread();

private:

    void WorkWithClient_();
    std::string GetStringFromClient_();
    std::string GetPieceFromClient_();

    ClientState state_ = ClientState::LOGIN;
    ServerClass* parent_;
    int socket_;
    //char buffer_[CSA::bufsize];

    boost::thread workingThread_;
    uint16_t port_;
    std::string IP_;

    void LogIn_();
};
}