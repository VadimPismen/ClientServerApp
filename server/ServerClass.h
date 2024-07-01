#include "../common.h"

class ServerClass
{
public:
    ServerClass(uint16_t port);
    ~ServerClass();

private:
    int serverSocket_;
    struct sockaddr_in addr_;
    uint16_t port_;
};