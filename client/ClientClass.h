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
        class KickedException{};

    private:

        std::string WriteAndSendToServer_();
        void SendStringToServer_(const std::string message);
        void SendPieceToServer_(const std::string message);
        std::string WriteString_();

        std::string GetStringFromServer_();
        std::string GetPieceFromServer_();

        void ParseCommand_(const std::string command);

        std::string IP_;
        uint16_t port_;
        std::string login_;

        ClientState state_ = ClientState::LOGIN;
        int socket_;
        struct sockaddr_in addr_;

    };
}