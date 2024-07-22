#pragma once

#include "../common.h"

namespace CSA
{
    class ClientClass
    {
    public:
        ClientClass(std::string cfgFile);
        ~ClientClass();

        void StartConnection();

        class KickedException{};

    private:

        std::string WriteSendGetString_();

        std::string WriteString_();

        void ParseCommand_(const std::string &command);
        inline void Disconnect_();

        std::string GetAbsolutePath_(const std::string path, const std::string base);

        std::string logsDir_;
        std::string IP_;
        int port_;
        std::string login_;

        std::string cfgFile_;
        libconfig::Config cfg_;
        std::string loadDir_ = std::filesystem::current_path().string() + "/" + DEFDIR;

        int socket_;
        struct sockaddr_in addr_;

    };
}