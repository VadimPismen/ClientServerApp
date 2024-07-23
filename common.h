#pragma once

#include <iostream>
#include <filesystem>
#include <regex>
#include <bitset>
#include <algorithm>
#include <map>

#include <boost/thread.hpp>
#include <boost/filesystem.hpp>

#include <fcntl.h>
#include <sys/stat.h>
#include <dirent.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include <stdint.h>

#define GLOG_NO_ABBREVIATED_SEVERITIES
#include <glog/logging.h>

#include <libconfig.h++>

namespace CSA
{
    // Размер буфера для получения сообщения по блокам
    const size_t BUFFSIZE = 1024;
    // Размер буфера для записи байтов длины сообщения
    const size_t LENSIZE = 10;
    // Размер блока чтения файла
    const size_t FILEBLOCKSIZE = 4096;
    // Регулярное выражение, используемое для проверки присутствия кавычек
    const std::regex LOADFILEREGEX("[\"](.*?)(?=[\"](\\s|$))");

    const std::string DEFDIR = "loads";
    const std::string UNDEFCOM = "Undefined command: ";
    const std::string NOACCESSTO = "No access to ";
    const std::string DIRECTORY = "directory";

    // Сигнатуры

    // Промежуточная информация
    const char INFO = 'I';
    // Загрузка файла
    const char LOAD = 'L';
    // Плохой результат
    const char BAD = 'B';
    // Успех. Сообщение должно быть пустым
    const char SUCCESS = 'S';
    
    // Перечисление команд, используемое сервером и клиентом для выбора действий
    enum class Commands{
        CD,
        CLEARLOGS,
        EXIT,
        HELP,
        LOADDIR,
        LOADFILE,
        GETDIRLIST,
        PROCS,
        SAVEDIR,
    };

    // Словарь, указывающий соответствие между строкой и командой
    const std::map<std::string, Commands> COMMANDS
    {
        {"cd", Commands::CD},
        {"clearlogs", Commands::CLEARLOGS},
        {"exit", Commands::EXIT},
        {"help", Commands::HELP},
        {"load", Commands::LOADFILE},
        {"loaddir", Commands::LOADDIR},
        {"ls", Commands::GETDIRLIST},
        {"procs", Commands::PROCS},
        {"savedir", Commands::SAVEDIR},
    };

    // Словарь, указывающий на соответствие команды строке помощи
    const std::map<Commands, std::string> HELPSTRINGS
    {
        {Commands::CD, "cd - show current directory;\n\
cd <path absolute or relative to the current directory> - change current directory;"},
        {Commands::EXIT, "exit - exit the program;"},
        {Commands::HELP, "help - help on all commands;\n\
help <name of command> - help on a specific command;"},
        {Commands::LOADFILE, "load <path to file> or \"path to file\" - load file to download directory;\n\
load \"path to file\" <filename> - load file to download directory and name it as <filename>;"},
        {Commands::LOADDIR, "loaddir - show current download directory;\n\
loaddir <path absolute or relative to the current download directory> - change current download directory;"},
        {Commands::GETDIRLIST, "ls -<args> - show a list of files in current directory;\n\
ls <path absolute or relative to the current directory> - show a list of files in directory;\n\
ls -<args> \"path absolute or relative to the current directory\" - show a list of files in directory with arguments;\n\
\ta\tShow hidden elements;\n\
\tF\tDo not show regular files;\n\
\tD\tDo not show directories"},
        {Commands::PROCS, "procs -<args> - show information about server's processes;\n\
\ta\tShow system processes;\n\
\tf\tShow information about descriptors;"},
        {Commands::SAVEDIR, "savedir - save current directory on account;"},
    };

    // Возможные исключения

    class ConnectionLostException{};
    class SocketCreationException{};
    class BindException{};

    class MessageObject
    {
    public:
        // Конструкторы
        MessageObject(char signature, std::string message):
        signature_(signature){
            sizeOfMessage_ = message.size();
            message_ = std::vector<char>(message.begin(), message.end());
        };

        MessageObject(char signature, std::vector<char> message):
        signature_(signature), message_(message){
            sizeOfMessage_ = message.size();
        };

        MessageObject(char signature, size_t sizeOfMessage, std::vector<char> message):
        signature_(signature), sizeOfMessage_(sizeOfMessage), message_(message){};

        ~MessageObject(){};
        
        // Геттеры
        char getSignature(){ return signature_;};
        size_t getsizeOfMessage(){ return sizeOfMessage_;};
        std::string getMessage(){ return std::string(message_.begin(), message_.end());};
        std::vector<char> getBytes(){ return message_;};

        // Отправить текстовое сообщение по сокету с определённой сигнатурой
        static ssize_t SendMessageObject(int socket, char signature, const std::string &message){
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

        // Отправить двоичное сообщение по сокету с определённой сигнатурой
        static ssize_t SendMessageObject(int socket, char signature, const std::vector<char> &message){
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

        // Отправить сообщение, содержащее c-строку
        static ssize_t SendMessageObject(int socket, char signature, const char *message, size_t sizeOfMessage){
            if (send(socket, &signature, sizeof(signature), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            if (send(socket, &sizeOfMessage, sizeof(sizeOfMessage), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            ssize_t sentBytes = 0;
            ssize_t allSentBytes = 0;
            while (sentBytes < sizeOfMessage){
                sentBytes = send(socket, message + allSentBytes, sizeOfMessage - allSentBytes, MSG_NOSIGNAL);
                if (sentBytes < 0){
                    throw ConnectionLostException();
                    return -1;
            }
            allSentBytes += sentBytes;
            }
            return allSentBytes;
        }

        // Отправить сообщение, содержащее только сигнатуру
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

        // Отправить объект сообщения по сети
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

        // Получить объект сообщения
        static MessageObject RecvMessageObject(int socket){
            char signature = '\0';
            size_t sizeOfMessage = 0;
            std::vector<char> message = {};
            if (recv(socket, &signature, sizeof(signature), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            if (recv(socket, &sizeOfMessage, sizeof(sizeOfMessage), MSG_NOSIGNAL) < 0){
                throw ConnectionLostException();
            }
            if (sizeOfMessage){
                ssize_t allGotBytes = 0;
                ssize_t gotBytes = 0;
                char buffer[BUFFSIZE];
                while (allGotBytes < sizeOfMessage){
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
        // Поля пакета:

        // Сигнатура
        char signature_ = '\0';
        // Размер сообщения
        size_t sizeOfMessage_ = 0;
        // Содержимое сообщения
        std::vector<char> message_ = {};
    };
}