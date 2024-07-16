#include "ServerClass.h"

int main()
{
    FLAGS_log_dir = std::filesystem::current_path().string() + "/logs";
    google::InitGoogleLogging("Server");
    CSA::ServerClass server("config.cfg");
    server.OpenServer();
    return 0;
}