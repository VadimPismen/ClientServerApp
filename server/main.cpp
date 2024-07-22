#include "ServerClass.h"

int main()
{
    CSA::ServerClass server("config.cfg");
    server.OpenServer();
    return 0;
}