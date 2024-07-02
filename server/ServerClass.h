#include "../common.h"

class ServerClass
{
public:
    ServerClass(uint16_t port);
    ~ServerClass();

    void OpenServer();

private:
    void WorkWithClient_(int socket);
    void SendToClient_(int __fd, const void *__buf, size_t __n);
    
    uint16_t port_;
    int serverSocket_;
    struct sockaddr_in addr_;

    std::map<int, boost::thread> listOfClients_;
};