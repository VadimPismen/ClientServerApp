#include "ServerClass.h"

int main()
{
    FLAGS_alsologtostderr = 1;
    FLAGS_log_dir = std::filesystem::current_path().string() + "/logs";
    google::InitGoogleLogging("Server");
    CSA::ServerClass server(1234);
    server.OpenServer();
    return 0;
}