#include "../common.h"
#include "ClientThread.h"

class ClientThread;

class ServerClass
{
public:
    ServerClass(uint16_t port);
    ~ServerClass();

    void OpenServer();

private:

    uint16_t port_;
    int serverSocket_;
    struct sockaddr_in addr_;

    std::vector<ClientThread> listOfClients_;
};