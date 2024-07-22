#include "ClientClass.h"

int main()
{
    CSA::ClientClass client("config.cfg");
    client.StartConnection();
    return 0;
}