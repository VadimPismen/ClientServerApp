#pragma once

#include <iostream>
#include <boost/thread.hpp>
#include <filesystem>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include <stdint.h>
#include <glog/logging.h>
#include <map>

namespace CSA
{
    const size_t bufsize = 1024;
    const std::string SIGN = "CSA";
    
    enum class ClientState{
        IDLE,
        LOGIN
    };

    namespace Order{
        const std::string CD = "cd";
    }
}