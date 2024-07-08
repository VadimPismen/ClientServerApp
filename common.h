#pragma once

#include <iostream>
#include <boost/thread.hpp>
#include <filesystem>
#include <boost/filesystem.hpp>

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
    const std::string ENDOFRES = "ENDOFRES";
    const std::string GOODBYE = "GOODBYE";
    const std::string DEFDIR = "DEFDIR";
    const std::string UNDEFCOM = "Undefined command: ";
    const std::string NOACCESSTO = "No access to ";
    const std::string DIRECTORY = "directory";
    
    enum class ClientState{
        IDLE,
        LOGIN,
        EXIT,
        WAITFORRESULT,
        LOADFILE
    };

    enum class Commands{
        EXIT,
        HELP,
        CD,
        SAVEDIR,
        LOADFILE
    };

    const std::map<std::string, Commands> COMMANDS
    {
        {"help", Commands::HELP},
        {"exit", Commands::EXIT},
        {"cd", Commands::CD},
        {"savedir", Commands::SAVEDIR},
        {"loadfile", Commands::LOADFILE}
    };

    const std::map<Commands, std::string> HELPSTRINGS
    {
        {Commands::HELP, "help - help on all commands;\n\
help <name of command> - help on a specific command;"},
        {Commands::EXIT, "exit - exit the program;"},
        {Commands::CD, "cd - show current directory;\n\
cd <absolute or relative path> - change current directory;"},
        {Commands::SAVEDIR, "savedir - save current directory on account;"},
        {Commands::LOADFILE, "loadfile \"path to file\" - load file to folder \"Downloads\";\n\
loadfile \"path to file\" <downloads folder> - load file from server to downloads folder."}
    };
}