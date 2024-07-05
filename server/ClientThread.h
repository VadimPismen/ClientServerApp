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

    void ParseCommand_(const size_t argsCount);

    std::string GetStringFromClient_();
    std::string GetPieceFromClient_();
    std::string GetArgsFromClient_();
    std::string GetArgsFromClient_(size_t countOfArgs);
    size_t GetCountOfArgsFromClient_();

    std::string ExecuteSystemCommandAndGetResult_(std::string command);

    void SendStringToClient_(std::string message);
    void SendPieceToClient_(std::string message);

    ClientState state_ = ClientState::LOGIN;
    ServerClass* parent_;
    int socket_;
    std::string name_;
    std::string clientCD_ = std::filesystem::current_path().string();

    boost::thread workingThread_;
    uint16_t port_;
    std::string IP_;

    bool LogIn_();
};
}