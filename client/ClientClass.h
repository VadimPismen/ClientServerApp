#pragma once

#include "../common.h"

namespace CSA
{
    // Класс клиента
    class ClientClass
    {
    public:
        // Инициализация клиента, конфигураций и логирования
        ClientClass(std::string cfgFile);
        ~ClientClass();

        // Начало подключения
        void StartConnection();

        class KickedException{};

    private:
        // Ввести строку и получить её
        std::string WriteSendGetString_();
        // Ввести строку, получить её и отправить серверу
        std::string WriteString_();

        // Парсинг команд
        void ParseCommand_(const std::string &command);
        // Отключение от сервера
        inline void Disconnect_();

        // Получить абсолютный путь
        std::string GetAbsolutePath_(const std::string path, const std::string base);

        // В онлайне ли клиент
        bool isOnline_ = true;
        // Директория логирования
        std::string logsDir_;
        std::string IP_;
        int port_;
        std::string login_;

        std::string cfgFile_;
        libconfig::Config cfg_;
        // Директория загрузок
        std::string loadDir_ = std::filesystem::current_path().string() + "/" + DEFDIR;

        int socket_;
        struct sockaddr_in addr_;

    };
}