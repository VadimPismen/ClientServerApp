#include "ServerClass.h"

ServerClass::ServerClass(uint16_t port): port_(port){}

ServerClass::~ServerClass(){
    close(serverSocket_);
}

void ServerClass::SendToClient_(int __fd, const void *__buf, size_t __n){
    if (send(__fd, __buf, __n, MSG_NOSIGNAL) < 0){
        throw boost::thread_interrupted();
    }
}

void ServerClass::WorkWithClient_(int socket){
    struct sockaddr_in clientData;
    socklen_t size = sizeof(clientData);

    getsockname(socket, (struct sockaddr*)&clientData, &size);

    std::string clientIP = inet_ntoa(clientData.sin_addr);
    uint16_t clientPort = htons(clientData.sin_port);

    LOG(INFO) << "new connection from " << clientIP << ':' << clientPort;

    char buffer[512];

    while(true){
        try{
            //recv(socket, &buffer, 512, 0);
            std::string hehe = "Not hehe";
            hehe.copy(buffer, 512);
            SendToClient_(socket, &buffer, 512);
        }
        catch(...){
            LOG(INFO) << "client " << clientIP << ':' << clientPort << " disconnected";
            close(socket);
            listOfClients_.erase(socket);
            return;
        }
    }
}

void ServerClass::OpenServer(){
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        perror("Socket creation error: ");
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "socket binding error!";
        perror("Bind error: ");
    }
    LOG(INFO) << "Server is opened on " << inet_ntoa(addr_.sin_addr) << ':' << port_;
    while(true){
        listen(serverSocket_, SOMAXCONN);
        int sock = accept(serverSocket_, NULL, NULL);
        if (sock >= 0){
            listOfClients_[sock] = boost::thread(&ServerClass::WorkWithClient_, this, sock);
        }
    }
}