#include "ClientThread.h"

using namespace CSA;

ClientThread::ClientThread(int socket, ServerClass* parent): socket_(socket), parent_(parent){
    struct sockaddr_in clientData;
    socklen_t size = sizeof(clientData);

    getsockname(socket, (struct sockaddr*)&clientData, &size);

    IP_ = inet_ntoa(clientData.sin_addr);
    port_ = htons(clientData.sin_port);

    LOG(INFO) << "new connection from " << IP_ << ':' << port_;

    boost::thread(&ClientThread::WorkWithClient_, this);
}

ClientThread::~ClientThread(){
    close(socket_);
    parent_->DeleteClient(socket_);
}

void ClientThread::WorkWithClient_(){
    while(true){
        switch (state_)
        {
        case ClientState::LOGIN:
            {
                LogIn_();
                break;
            }
        default:
            break;
        }
        
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

void ClientThread::LogIn_(){
    std::string login = GetStringFromClient_();
    LOG(INFO) << IP_ << ':' << port_ << " is " << login;
    std::string password = GetStringFromClient_();
    LOG(INFO) << IP_ << ':' << port_ << " is " << password;
}

std::string ClientThread::GetPieceFromClient_(){
    char buffer[CSA::bufsize];
    ssize_t countOfData = recv(socket_, &buffer, CSA::bufsize, MSG_NOSIGNAL);
        if (countOfData < 0){
            LOG(WARNING) << "lost connection to " << IP_ << ':' << port_;
            this->~ClientThread();
        }
    return std::string(buffer);
}

// ssize_t clientData = recv(socket_, &buffer_, CSA::bufsize, MSG_NOSIGNAL);
//         if (clientData > 0){
//               LOG(INFO) << IP_ << ':' << port_ << " says: " << buffer_;
//         }
//         else{
//             LOG(WARNING) << "lost connection to " << IP_ << ':' << port_;
//             this->~ClientThread();
//         }


// void ClientThread::SendToClient_(const std::string message){
//     buffer = std::string
//     ssize_t clientData = recv(socket_, &buffer_, CSA::bufsize, MSG_NOSIGNAL);
//     if (send(__fd, __buf, __n, MSG_NOSIGNAL) < 0){
//         throw boost::thread_interrupted();
//     }
// }