#include "ClientClass.h"

int main()
{
    FLAGS_alsologtostderr = 1;
    FLAGS_log_dir = std::filesystem::current_path().string() + "/logs";
    google::InitGoogleLogging("Client");
    CSA::ClientClass client("192.168.4.201", 1234);
    client.StartConnection();
    return 0;
}