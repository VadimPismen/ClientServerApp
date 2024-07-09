#include "ClientClass.h"

using namespace CSA;

ClientClass::ClientClass(std::string IP, uint16_t port): IP_(IP), port_(port){
    // cfg_.readFile("config.cfg");
    // libconfig::Setting& root_ =  cfg_.getRoot();
};

ClientClass::~ClientClass(){
    close(port_);
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
                        std::string result;
                        while(result != ENDOFRES){
                            std::cout << result << std::endl;
                            result = GetStringFromServer_();
                        }
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

                std::string command = WriteString_();
                SendStringToServer_(command);
                if (IsSpecCommand_(command)){
                    break;
                }
                std::string result = GetStringFromServer_();
                if (result == GOODBYE){
                    LOG(INFO) << "Exiting...";
                    this->~ClientClass();
                    return;
                    break;
                }
                while(result != ENDOFRES){
                    std::cout << result << std::endl;
                    result = GetStringFromServer_();
                }
                break;
            }
            case ClientState::LOADFILE:
            {
                std::string filename = GetStringFromServer_();
                std::string dir = GetStringFromServer_();
                if (dir == ENDOFRES){
                    std::cout << filename << std::endl;
                    state_ = ClientState::IDLE;
                    break;
                }
                if (dir == ""){
                    dir = "loads";
                }
                std::ofstream loadedFile(dir + "/" + filename, std::ios_base::binary);
                if (loadedFile.fail()){
                    SendStringToServer_(BADRES);
                    std::cout << "Bad directory" << std::endl;
                    loadedFile.close();
                    state_ = ClientState::IDLE;
                    break;
                }
                else{
                    SendStringToServer_(SUCCESS);
                    ssize_t countOfByteBlocks = std::stoull(GetPieceFromServer_());
                    ssize_t sizeOfByteTail = std::stoull(GetPieceFromServer_());
                    char data[BUFFSIZE];
                    for (ssize_t i = 1; i < countOfByteBlocks; i++){
                        GetBytesFromServerToCharArray_(data);
                        loadedFile.write(data,BUFFSIZE);
                    }
                    GetBytesFromServerToCharArray_(data);
                    loadedFile.write(data,sizeOfByteTail);
                    loadedFile.close();
                    std::cout << "File is loaded to " << dir + "/" + filename << std::endl;
                    state_ = ClientState::IDLE;
                }
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
    while (message == ""){
        getline(std::cin, message);
    }
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
    size_t sizeOfData;
    try{
        sizeOfData = std::stoull(GetPieceFromServer_());
    }
    catch(...){
        sizeOfData = 0;
    }
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

ssize_t ClientClass::GetBytesFromServerToCharArray_(char* buffer){
    ssize_t countOfData = recv(socket_, buffer, CSA::BUFFSIZE, MSG_NOSIGNAL);
        if (countOfData < 0){
            throw ConnectionLostException();
        }
    return countOfData;
}

bool ClientClass::IsSpecCommand_(const std::string command){
    if (command.empty()){
        return false;
    }
    std::istringstream ss(command);
    std::string firstArg;
    ss >> firstArg;
    try{
        const Commands com = COMMANDS.at(firstArg);
        switch (com)
        {
            case Commands::LOADFILE:
            {
                state_ = ClientState::LOADFILE;
                return true;
                break;
            }
            default:
            {
                return false;
                break;
            }
        }
    }
    catch(...){
        return false;
    }
    return false;
}