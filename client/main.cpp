#include "ClientClass.h"

int main()
{
    FLAGS_logtostderr = 0;
    FLAGS_log_dir = std::filesystem::current_path().string() + "/logs";
    google::InitGoogleLogging("Client");
    ClientClass client("192.168.4.201", 1234);
    client.StartConnection();
    return 0;
}