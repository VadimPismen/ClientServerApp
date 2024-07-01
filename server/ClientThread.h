#pragma once

#include "../common.h"

class ClientThread
{
public:
    ClientThread(int socket);
    ~ClientThread();

private:
    void WorkWithClient();
    int socket_;
    char buffer[512];
};