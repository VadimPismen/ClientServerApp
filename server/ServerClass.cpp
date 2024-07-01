#include "ServerClass.h"
#include "../common.h"

ServerClass::ServerClass(uint16_t port): port_(port){}

ServerClass::~ServerClass(){
    close(serverSocket_);
}

void ServerClass::OpenServer(){
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        perror("Socket creation error: ");
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        perror("Bind error");
    }
    while(true){
        listen(serverSocket_, SOMAXCONN);
        int sock = accept(serverSocket_, NULL, NULL);
        if (sock >= 0){
            listOfClients_.push_back(ClientThread{sock});
            }
    }
}