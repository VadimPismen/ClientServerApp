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

#include <libconfig.h++>

namespace CSA
{
    const size_t BUFFSIZE = 1024;
    const std::string SIGN = "CSA";
    const std::string SUCCESS = "SUCCESS";
    const std::string GETOUT = "GETOUT";
    const std::string BADRES = "BADRES";
    
    enum class ClientState{
        IDLE,
        LOGIN,
        EXIT,
        WAITFORRESULT
    };

    enum class Commands{
        EXIT,
        HELP,
        CD
    };

    const std::map<std::string, Commands> COMMANDS
    {
        {"help", Commands::HELP},
        {"exit", Commands::EXIT},
        {"chdir", Commands::CD}
    };

    const std::map<Commands, std::string> HELPSTRINGS
    {
        {Commands::HELP, "help - help on all commands;\n\
help <name of command> - help on a specific command;"},
        {Commands::EXIT, "exit - exit the program"},
        {Commands::CD, "cd - show current directory;"}
    };
}