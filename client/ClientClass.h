#pragma once

#include "../common.h"

namespace CSA
{
    class ClientClass
    {
    public:
        ClientClass(std::string IP, uint16_t port);
        ~ClientClass();

        void StartConnection();

        class KickedException{};

    private:

        std::string WriteSendGetString_();

        std::string WriteString_();

        void ParseCommand_(const std::string &command);
        inline void Disconnect_();

        std::string GetAbsolutePath_(const std::string path);

        std::string IP_;
        uint16_t port_;
        std::string login_;

        std::string cfgFile_;
        libconfig::Config cfg_;
        std::string loadDir_ = std::filesystem::current_path().string() + "/" + DEFDIR;

        int socket_;
        struct sockaddr_in addr_;

    };
}