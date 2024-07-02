#include "ServerClass.h"

int main()
{
    FLAGS_logtostderr = 0;
    FLAGS_log_dir = std::filesystem::current_path().string() + "/logs";
    google::InitGoogleLogging("Server");
    ServerClass server(1234);
    server.OpenServer();
    return 0;
}