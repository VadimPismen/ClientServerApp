#include "ClientClass.h"

using namespace CSA;

ClientClass::ClientClass(std::string IP, uint16_t port): IP_(IP), port_(port){};

ClientClass::~ClientClass(){
    close(socket_);
}

void ClientClass::StartConnection() {
     socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        perror("Socket creation error: ");
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = inet_addr(IP_.c_str());
    if (connect(socket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "connection to " << IP_ << ':' << port_ << " is failed!";
        perror("Connection error: ");
    }
    LOG(INFO) << "connected to " << IP_ << ':' << port_;

    try{
        while(true){
            switch (state_)
            {
            case ClientState::LOGIN:
            {
                while(true){
                    std::cout << "Login: ";
                    login_ = WriteAndSendToServer_();

                    std::cout << "Password: ";
                    WriteAndSendToServer_();

                    std::string result = GetStringFromServer_();
                    
                    if (result == SUCCESS){
                        std::cout << "Successful login!" << std::endl;
                        LOG(INFO) << "Successful login as " << login_;
                        state_ = ClientState::IDLE;
                        break;
                    }
                    else{
                        if (result != GETOUT){
                            std::cout << result << std::endl << std::endl;
                            LOG(INFO) << "Unsuccessful attempt.";
                        }
                        else{
                            throw KickedException();
                        }
                    }
                }
                break;
            }
            case ClientState::EXIT:
            {
                LOG(INFO) << "Exiting...";
                this->~ClientClass();
                return;
                break;
            }
            case ClientState::IDLE:
            {
                ParseCommand_(WriteString_());
                break;
            }
            case ClientState::WAITFORRESULT:
            {
                std::string result = GetStringFromServer_();
                std::cout << result << std::endl;
                break;
            }
            default:
                break;
            }
        }
    }
    catch(ConnectionLostException){
        LOG(ERROR) << "lost connection to server " << IP_ << ':' << port_;
        std::cout << "Connection was lost." << std::endl;
        this->~ClientClass();
        return;
    }
    catch(KickedException){
        std::cout << "Alas!";
        LOG(WARNING) << "couldn't login... Disconnect.";
        this->~ClientClass();
        return;
    }
}

std::string ClientClass::WriteString_(){
    std::string message;
    getline(std::cin, message);
    return message;
}


std::string ClientClass::WriteAndSendToServer_(){
    std::string message;
    getline(std::cin, message);
    SendStringToServer_(message);
    return message;
}

void ClientClass::SendPieceToServer_(const std::string message){
    const char* buffer = message.c_str();
    if (send(socket_, buffer, CSA::BUFFSIZE, MSG_NOSIGNAL) < 0){
        throw ConnectionLostException();
    }
}

void ClientClass::SendStringToServer_(const std::string message){
    SendPieceToServer_(CSA::SIGN);
    size_t sizeOfData = message.length();
    SendPieceToServer_(std::to_string(sizeOfData) + '\n');
    size_t countOfBlocks = sizeOfData / CSA::BUFFSIZE;
    if (sizeOfData % CSA::BUFFSIZE != 0){
        countOfBlocks++;
    }
    for (size_t i = 0; i < countOfBlocks; i++){
        SendPieceToServer_(message.substr(i*CSA::BUFFSIZE, CSA::BUFFSIZE));
    }
}

std::string ClientClass::GetStringFromServer_(){
    size_t sizeOfData = std::stoull(GetPieceFromServer_());
    std::string serverData = "";
    size_t countOfBlocks = sizeOfData / CSA::BUFFSIZE;
    if (sizeOfData % CSA::BUFFSIZE != 0){
        countOfBlocks++;
    }
    for (size_t i = 0; i < countOfBlocks; i++){
        serverData += GetPieceFromServer_();
    }
    return serverData;
}

std::string ClientClass::GetPieceFromServer_(){
    char buffer[CSA::BUFFSIZE];
    ssize_t countOfData = recv(socket_, &buffer, CSA::BUFFSIZE, MSG_NOSIGNAL);
        if (countOfData < 0){
            throw ConnectionLostException();
        }
    return std::string(buffer);
}

void ClientClass::ParseCommand_(const std::string command){
    if (command.empty()){
        return;
    }
    std::vector<std::string> args;
    std::istringstream ss(command);
    std::string arg;
    while (ss >> arg) 
    {
        args.push_back(arg);
    }
    try{
        const Commands com = COMMANDS.at(args[0]);
        switch (com)
        {
            case Commands::EXIT:
            {
                SendStringToServer_("1");
                SendStringToServer_(args[0]);
                state_ = ClientState::EXIT;
                break;
            }
            case Commands::HELP:
            {
                if (args.size() == 1){
                    for(auto& helpStr : HELPSTRINGS){
                        std::cout << helpStr.second << std::endl;
                    }
                    std::cout << std::endl;
                }
                else{
                    try
                    {
                        std::cout << HELPSTRINGS.at(COMMANDS.at(args[1])) << std::endl << std::endl;
                    } catch(std::out_of_range){
                    std::cout << "Unknown command.";
                    }
                }
                break;
            }
            case Commands::CD:
            {
                SendStringToServer_(std::to_string(args.size()));
                SendStringToServer_(command);
                state_ = ClientState::WAITFORRESULT;
                break;
            }
        }
    } catch(std::out_of_range){
        std::cout << "Unknown command.";
    }
    return;
}