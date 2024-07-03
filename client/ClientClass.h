#include "../common.h"

namespace CSA
{
    class ClientClass
    {
    public:
        ClientClass(std::string IP, uint16_t port);
        ~ClientClass();

        void StartConnection();

        class ConnectionLostException{};

    private:

        void SendStringToServer_(const std::string message);
        void SendPieceToServer_(const std::string message);

        std::string IP_;
        uint16_t port_;

        ClientState state_ = ClientState::LOGIN;
        int socket_;
        struct sockaddr_in addr_;

    };
}