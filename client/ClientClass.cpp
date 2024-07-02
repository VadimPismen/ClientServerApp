#include "ClientClass.h"

ClientClass::ClientClass(std::string IP, uint16_t port): IP_(IP), port_(port){};

ClientClass::~ClientClass(){
    close(clientSocket_);
}

void ClientClass::StartConnection() {
     clientSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        perror("Socket creation error: ");
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = inet_addr(IP_.c_str());
    if (connect(clientSocket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "connection to " << IP_ << ':' << port_ << " is failed!";
        perror("Connection error: ");
    }
    LOG(INFO) << "connected to " << IP_ << ':' << port_;
    inputThread_ = boost::thread(&ClientClass::WritingInput_, this);
    char buffer[512];
    while(true){
        try{
            //SendToServer_(clientSocket_, &buffer, 512);
            recv(clientSocket_, &buffer, 512, 0);
            std::cout << buffer << std::endl;
        }
        catch(...){
            LOG(WARNING) << "connection lost";
            close(clientSocket_);
            return;
        }
    }
}

void ClientClass::SendToServer_(int __fd, const void *__buf, size_t __n){
    if (send(__fd, __buf, __n, MSG_NOSIGNAL) < 0){
        throw ConnectionLostException();
    }
}

void ClientClass::GetFromServer_(int __fd, void *__buf, size_t __n){
    recv(__fd, __buf, __n, 0);
    return;
}

void ClientClass::WritingInput_() {
    while(true){
        sleep(0.1);
    }
};
