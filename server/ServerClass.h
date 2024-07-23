#pragma once

#include "../common.h"
#include "ClientThread.h"

namespace CSA
{
    class ClientThread;
    
    // Класс сервера
    class ServerClass
    {
    public:
        // Инициализация сервера, конфига и логирования
        ServerClass(std::string cfgFile);
        ~ServerClass();

        // Открыть подключение
        void OpenServer();
        // Отключить клиента по сокету
        void DeleteClient(int socket, std::string logMessage);

        // Найти аккаунт в базе по имени и паролю
        bool LookForAccount(std::string login, std::string password);
        // Получить текущую директорию аккаунта из базы
        std::string GetUserDirFromConfs(std::string login);
        // Сохранить текущую директорию аккаунта в базу
        void SaveUserDir(std::string login, std::string dir);
        

    private:

        // Парсинг команд
        void GetAdminCommands_();
        // Получить абсолютный путь
        std::string GetAbsolutePath_(const std::string path, const std::string base);

        // Поток исполнения для отправки команд
        boost::thread adminCommandsThread_;
        // Файл конфигураций
        std::string cfgFile_;
        // Конфигурации
        libconfig::Config cfg_;
        
        // Директория логирования
        std::string logsDir_;

        // Включен ли сервер
        bool isOnline_ = true;
        int port_;
        int serverSocket_;
        struct sockaddr_in addr_;

        // Список клиентов
        std::map<int, ClientThread> listOfClients_;
    };
}