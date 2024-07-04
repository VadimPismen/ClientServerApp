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

    class ConnectionLostException{};

private:
    void WorkWithClient_();

    std::string GetStringFromClient_();
    std::string GetPieceFromClient_();

    void SendStringToClient_(std::string message);
    void SendPieceToClient_(std::string message);

    ClientState state_ = ClientState::LOGIN;
    ServerClass* parent_;
    int socket_;
    //char buffer_[CSA::bufsize];

    boost::thread workingThread_;
    uint16_t port_;
    std::string IP_;

    bool LogIn_();
};
}