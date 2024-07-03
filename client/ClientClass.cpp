#include "ClientClass.h"

using namespace CSA;

ClientClass::ClientClass(std::string IP, uint16_t port): IP_(IP), port_(port){};

ClientClass::~ClientClass(){
    close(socket_);
}

void ClientClass::StartConnection() {
     socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        perror("Socket creation error: ");
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = inet_addr(IP_.c_str());
    if (connect(socket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "connection to " << IP_ << ':' << port_ << " is failed!";
        perror("Connection error: ");
    }
    LOG(INFO) << "connected to " << IP_ << ':' << port_;
    while(true){
        switch (state_)
        {
        case ClientState::LOGIN:
        {
            std::string message;
            getline(std::cin, message);
            SendStringToServer_(message);
            break;
        }
        default:
            break;
        }
    }
}

void ClientClass::SendPieceToServer_(const std::string message){
    const char* buffer = message.c_str();
    if (send(socket_, buffer, CSA::bufsize, MSG_NOSIGNAL) < 0){
        throw ConnectionLostException();
    }
}

void ClientClass::SendStringToServer_(const std::string message){
    SendPieceToServer_(CSA::SIGN);
    size_t sizeOfData = message.length();
    SendPieceToServer_(std::to_string(sizeOfData) + '\n');
    size_t countOfBlocks = sizeOfData / CSA::bufsize;
    if (sizeOfData % CSA::bufsize != 0){
        countOfBlocks++;
    }
    for (size_t i = 0; i < countOfBlocks; i++){
        SendPieceToServer_(message.substr(i*CSA::bufsize, CSA::bufsize));
    }
}