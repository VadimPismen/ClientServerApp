#pragma once

#include "../common.h"
#include "ServerClass.h"

namespace CSA
{
        
    class ServerClass;

    // Класс клиентского потока
    class ClientThread{

    public:

        ClientThread();
        ~ClientThread();

        // Начало работы с клиентом
        void StartWorking(int socket, ServerClass* parent);
        std::string getIP() const { return IP_;};
        std::string getName() const { return name_;};

    private:
        // Основной цикл работы с клиентом
        void WorkWithClient_();
        // Отключить клиента
        void DisconnectClient_();
        // Отключить клиента при потере соединения
        void LostConnection_();

        // Парсинг команд
        void ParseCommand_(const std::string command);

        // Перехват проблем с файлом на стороне клиента
        void interceptLoadFileInterruption_(bool &isNotInterrupted);

        [[deprecated("Unused")]]
        std::string ExecuteSystemCommandAndGetResult_(const std::string command);

        // Получить абсолютный путь
        std::string GetAbsolutePath_(const std::string path);

        ServerClass* parent_;
        int socket_;
        std::string name_;
        std::string clientCD_ = std::string(get_current_dir_name());
        bool canWrite_ = true;
        bool isOnline_ = true;

        // Поток работы с клиентом
        boost::thread workingThread_;
        // Поток перехвата ошибок с файлом
        boost::thread badLoadThread_;
        uint16_t port_;
        std::string IP_;

    };
}