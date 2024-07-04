#include "ServerClass.h"

int main()
{
    FLAGS_logtostderr = 0;
    FLAGS_log_dir = std::filesystem::current_path().string() + "/logs";
    google::InitGoogleLogging("Server");
    CSA::ServerClass server(1234, "config.cfg");
    server.OpenServer();
    return 0;
}