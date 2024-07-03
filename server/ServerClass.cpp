#include "ServerClass.h"

using namespace CSA;

ServerClass::ServerClass(uint16_t port): port_(port){}

ServerClass::~ServerClass(){
    close(serverSocket_);
}

void ServerClass::OpenServer(){
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        perror("Socket creation error: ");
        exit(1);
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "socket binding error!";
        perror("Bind error: ");
        exit(1);
    }
    LOG(INFO) << "Server is opened on " << inet_ntoa(addr_.sin_addr) << ':' << port_;
    while(true){
        listen(serverSocket_, SOMAXCONN);
        int sock = accept(serverSocket_, NULL, NULL);
        if (sock >= 0){
            listOfClients_.emplace(std::piecewise_construct, std::forward_as_tuple(sock), std::forward_as_tuple(sock, this)); 
        }
    }
}

void ServerClass::DeleteClient(int socket){
    listOfClients_.erase(socket);
}