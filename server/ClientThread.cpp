#include "ClientThread.h"

using namespace CSA;

ClientThread::ClientThread(){}

ClientThread::~ClientThread(){
    close(socket_);
}

void ClientThread::StartWorking(int socket, ServerClass* parent){
    socket_ = socket;
    parent_ = parent;

    struct sockaddr_in clientData;
    socklen_t size = sizeof(clientData);

    getsockname(socket, (struct sockaddr*)&clientData, &size);

    IP_ = inet_ntoa(clientData.sin_addr);
    port_ = htons(clientData.sin_port);

    LOG(INFO) << "new connection from " << IP_ << ':' << port_;
    workingThread_ = boost::thread(&ClientThread::WorkWithClient_, this);
    return;
}

void ClientThread::WorkWithClient_(){
    try{
        while(true){
            switch (state_)
            {
            case ClientState::LOGIN:
                {
                    if (LogIn_()){
                        state_ == ClientState::IDLE;
                    }
                    else{
                        LOG(WARNING) << IP_ << ':' << port_ << ": couldn't login. Kicked!";
                        parent_->DeleteClient(socket_);
                        return;
                    }
                    break;
                }
            default:
                break;
            }
        }
    }
    catch(...){
        LOG(WARNING) << "lost connection to " << IP_ << ':' << port_;
        parent_->DeleteClient(socket_);
        return;
    }
}

void ClientThread::SendPieceToClient_(const std::string message){
    const char* buffer = message.c_str();
    if (send(socket_, buffer, CSA::bufsize, MSG_NOSIGNAL) < 0){
        throw ConnectionLostException();
    }
}

void ClientThread::SendStringToClient_(const std::string message){
    size_t sizeOfData = message.length();
    SendPieceToClient_(std::to_string(sizeOfData) + '\n');
    size_t countOfBlocks = sizeOfData / CSA::bufsize;
    if (sizeOfData % CSA::bufsize != 0){
        countOfBlocks++;
    }
    for (size_t i = 0; i < countOfBlocks; i++){
        SendPieceToClient_(message.substr(i*CSA::bufsize, CSA::bufsize));
    }
}

std::string ClientThread::GetStringFromClient_(){
    std::string signature = GetPieceFromClient_();
    if (signature != SIGN){
        LOG(WARNING) << IP_ << ':' << port_ << " kicked out as an outsider!";
        this->~ClientThread();
    }
    size_t sizeOfData = std::stoull(GetPieceFromClient_());
    std::string clientData = "";
    size_t countOfBlocks = sizeOfData / CSA::bufsize;
    if (sizeOfData % CSA::bufsize != 0){
        countOfBlocks++;
    }
    for (size_t i = 0; i < countOfBlocks; i++){
        clientData += GetPieceFromClient_();
    }
    return clientData;
}

bool ClientThread::LogIn_(){
    unsigned char attempts = 3;
    while(true){
        std::string login = GetStringFromClient_();
        std::string password = GetStringFromClient_();
        if (parent_->LookForAccount(login, password)){
            SendStringToClient_(SUCCESS);
            LOG(INFO) << IP_ << ':' << port_ << " is " << login;
            return true;
        }
        else{
            attempts--;
            if (attempts){
                SendStringToClient_(std::to_string(attempts) + " attempts left.");
                LOG(INFO) << IP_ << ':' << port_ << ": unsuccessful login attempt.";
            }
            else{
                SendStringToClient_(GETOUT);
                return false;
            }
        }
    }
}

std::string ClientThread::GetPieceFromClient_(){
    char buffer[CSA::bufsize];
    ssize_t countOfData = recv(socket_, &buffer, CSA::bufsize, MSG_NOSIGNAL);
        if (countOfData < 0){
            throw ConnectionLostException();
        }
    return std::string(buffer);
}