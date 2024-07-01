#include "ServerClass.h"
#include "../common.h"

ServerClass::ServerClass(uint16_t port): port_(port){
serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        perror("Socket creation error: ");
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(3425);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        perror("Bind error");
    }
    listen(serverSocket_, 10);
}

ServerClass::~ServerClass(){
    close(serverSocket_);
}