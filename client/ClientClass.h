#include "../common.h"

class ClientClass
{
public:
    ClientClass(std::string IP, uint16_t port);
    ~ClientClass();

    void StartConnection();

    class ConnectionLostException{};

private:

    void WritingInput_();
    void SendToServer_(int __fd, const void *__buf, size_t __n);
    void GetFromServer_(int __fd, void *__buf, size_t __n);

    std::string IP_;
    uint16_t port_;

    int clientSocket_;
    struct sockaddr_in addr_;

    boost::thread inputThread_;
};