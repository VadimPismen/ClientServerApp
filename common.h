#pragma once

#include <iostream>
#include <filesystem>
#include <regex>

#include <boost/thread.hpp>
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
    const size_t LENSIZE = 10;
    const size_t FILEBLOCKSIZE = 4096;
    const std::regex LOADFILEREGEX("[\"](.*?)(?=[\"](\\s|$))");

    const std::string SIGN = "CSA";
    const std::string DEFDIR = "loads";
    const std::string UNDEFCOM = "Undefined command: ";
    const std::string NOACCESSTO = "No access to ";
    const std::string DIRECTORY = "directory";

    const char INFO = 'I';
    const char SUCCESS = 'S';
    const char GETOUT = 'O';
    const char GOODBYE = 'G';
    const char LOAD = 'L';
    
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
        {"load", Commands::LOADFILE}
    };

    const std::map<Commands, std::string> HELPSTRINGS
    {
        {Commands::HELP, "help - help on all commands;\n\
help <name of command> - help on a specific command;"},
        {Commands::EXIT, "exit - exit the program;"},
        {Commands::CD, "cd - show current directory;\n\
cd <absolute or relative path> - change current directory;"},
        {Commands::SAVEDIR, "savedir - save current directory on account;"},
        {Commands::LOADFILE, "load \"path to file\" - load file to folder \"Downloads\";"}
    };

    class ConnectionLostException{};

    class MessageObject
    {
    public:
        MessageObject(char signature, std::string message):
        signature_(signature){
            sizeOfMessage_ = message.size();
            message_ = std::vector<char>(message.begin(), message.end());
        };

        MessageObject(char signature, std::vector<char> message):
        signature_(signature), message_(message){
            sizeOfMessage_ = message.size();
        };

        MessageObject(char signature, size_t sizeOfMessage, std::string message):
        signature_(signature), sizeOfMessage_(sizeOfMessage){
            message_ = std::vector<char>(message.begin(), message.end());
        };


        MessageObject(char signature, size_t sizeOfMessage, std::vector<char> message):
        signature_(signature), sizeOfMessage_(sizeOfMessage), message_(message){};

        ~MessageObject(){};
        
        char getSignature(){ return signature_;};
        size_t getsizeOfMessage(){ return sizeOfMessage_;};
        std::string getMessage(){ return std::string(message_.begin(), message_.end());};
        std::vector<char> getBytes(){ return message_;};


        static ssize_t SendMessageObject(int socket, char signature, std::string message){
            size_t sizeOfMessage = message.size();
            if (send(socket, &signature, sizeof(signature), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            if (send(socket, &sizeOfMessage, sizeof(sizeOfMessage), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            ssize_t sentBytes = 0;
            ssize_t allSentBytes = 0;
            while (sentBytes < sizeOfMessage){
                sentBytes = send(socket, message.data() + allSentBytes, sizeOfMessage - allSentBytes, MSG_NOSIGNAL);
                if (sentBytes < 0){
                    throw ConnectionLostException();
                    return -1;
            }
            allSentBytes += sentBytes;
            }
            return allSentBytes;
        }

        static ssize_t SendMessageObject(int socket, char signature, std::vector<char> message){
            size_t sizeOfMessage = message.size();
            if (send(socket, &signature, sizeof(signature), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            if (send(socket, &sizeOfMessage, sizeof(sizeOfMessage), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            ssize_t sentBytes = 0;
            ssize_t allSentBytes = 0;
            while (sentBytes < sizeOfMessage){
                sentBytes = send(socket, message.data() + allSentBytes, sizeOfMessage - allSentBytes, MSG_NOSIGNAL);
                if (sentBytes < 0){
                    throw ConnectionLostException();
                    return -1;
            }
            allSentBytes += sentBytes;
            }
            return allSentBytes;
        }

        static ssize_t SendMessageObject(int socket, char signature){
            size_t sizeOfMessage = 0;
            if (send(socket, &signature, sizeof(signature), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            if (send(socket, &sizeOfMessage, sizeof(sizeOfMessage), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            return 0;
        }

        ssize_t SendMessageObject(int socket){
            if (send(socket, &signature_, sizeof(signature_), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            if (send(socket, &sizeOfMessage_, sizeof(sizeOfMessage_), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            ssize_t sentBytes = 0;
            ssize_t allSentBytes = 0;
            while (sentBytes < sizeOfMessage_){
                sentBytes = send(socket, message_.data() + allSentBytes, sizeOfMessage_ - allSentBytes, MSG_NOSIGNAL);
                if (sentBytes < 0){
                    throw ConnectionLostException();
                    return -1;
            }
            allSentBytes += sentBytes;
            }
            return allSentBytes;
        }

        static MessageObject RecvMessageObject(int socket){
            char signature = '\0';            
            if (recv(socket, &signature, sizeof(signature), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            size_t sizeOfMessage = 0;
            if (recv(socket, &sizeOfMessage, sizeof(sizeOfMessage), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            std::vector<char> message = {};
            if (sizeOfMessage){
                ssize_t allGotBytes = 0;
                ssize_t gotBytes = 0;
                char buffer[BUFFSIZE];
                while (gotBytes < sizeOfMessage){
                    ssize_t bytesToGetNow = ((sizeOfMessage - allGotBytes) < CSA::BUFFSIZE) ? sizeOfMessage - allGotBytes : CSA::BUFFSIZE;
                    gotBytes = recv(socket, &buffer, bytesToGetNow, MSG_NOSIGNAL);
                    if (gotBytes < 0){
                        throw ConnectionLostException();
                    }
                    allGotBytes += gotBytes;
                    std::copy(buffer, &buffer[gotBytes], back_inserter(message));
                }
            }
            return MessageObject(signature, sizeOfMessage, message);
        }

    private:
        char signature_ = '\0';
        size_t sizeOfMessage_ = 0;
        std::vector<char> message_ = {};
    };
}