#include "ClientClass.h"

using namespace CSA;

ClientClass::ClientClass(std::string IP, uint16_t port): IP_(IP), port_(port){
    // cfg_.readFile("config.cfg");
    // libconfig::Setting& root_ =  cfg_.getRoot();
};

ClientClass::~ClientClass(){
    close(port_);
    exit(0);
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
            std::cout << "Login: ";
            login_ = WriteSendGetString_();

            std::cout << "Password: ";
            WriteSendGetString_();

            MessageObject recv = MessageObject::RecvMessageObject(socket_);
            
            if (recv.getSignature() == SUCCESS){
                std::cout << "Successful login!" << std::endl;
                LOG(INFO) << "Successful login as " << login_;
                while(true){
                    MessageObject result = MessageObject::RecvMessageObject(socket_);
                    std::cout << result.getMessage() << std::endl;
                    if (result.getSignature() == SUCCESS){
                        break;
                    };
                }
                break;
            }
            else{
                if (recv.getSignature() != GETOUT){
                    std::cout << recv.getMessage() << std::endl << std::endl;
                    LOG(INFO) << "Unsuccessful attempt.";
                }
                else{
                    throw KickedException();
                }
            }
        }
        while(true){
            ParseCommand_(WriteString_());
        }
    }
    catch(ConnectionLostException){
        LOG(ERROR) << "lost connection to server " << IP_ << ':' << port_;
        std::cout << "Connection was lost." << std::endl;
        Disconnect_();
        return;
    }
    catch(KickedException){
        std::cout << "Alas!";
        LOG(WARNING) << "couldn't login... Disconnect.";
        Disconnect_();
        return;
    }
}

std::string ClientClass::WriteString_(){
    std::string message;
    getline(std::cin, message);
    return message;
}

std::string ClientClass::WriteSendGetString_(){
    std::string message;
    while (message == ""){
        getline(std::cin, message);
    }
    MessageObject::SendMessageObject(socket_, INFO, message);
    return message;
}

void ClientClass::ParseCommand_(const std::string &command)
{
    if (command.empty()){
        return;
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
                std::smatch match;
                std::regex_search(command, match, CSA::LOADFILEREGEX);
                std::string serverFileAdr = match.str();
                if (serverFileAdr.empty()){
                    std::cout << "Address of server file must be in \"" << std::endl;
                    break;
                }
                std::string clientFileName = command.substr(match.position() + serverFileAdr.length() + 1);
                serverFileAdr.erase(0,1);
                while (clientFileName[0] == ' '){
                    clientFileName.erase(0, 1);
                }
                if (clientFileName.empty()){
                    std::size_t begOfName = serverFileAdr.find_last_of('/');
                    if (begOfName != std::string::npos){
                        clientFileName = serverFileAdr.substr(begOfName + 1);
                    }
                    else{
                        clientFileName = serverFileAdr;
                    }
                }
                std::ofstream clientFile(loadDir_ + '/' + clientFileName);
                if (clientFile.is_open(), std::ios_base::binary){
                    MessageObject::SendMessageObject(socket_, INFO, firstArg + " " + serverFileAdr);
                    MessageObject result = MessageObject::RecvMessageObject(socket_);
                    if (result.getSignature() == SUCCESS){
                        while(true){
                            result = MessageObject::RecvMessageObject(socket_);
                            if (result.getSignature() == LOAD){
                                clientFile.write(result.getBytes().data(), result.getsizeOfMessage());
                            }
                            else
                            {
                                std::cout << "File was loaded to " << loadDir_ + '/' + clientFileName << std::endl;
                                clientFile.close();
                                break;
                            }
                        }
                    }
                    else{
                        std::cout << result.getMessage() << std::endl;
                        clientFile.close();
                        break;
                    }
                }
                else{
                    std::cout << "Cannot create file!" << std::endl;
                }
                break;
            }
            case Commands::EXIT:
            {
                MessageObject::SendMessageObject(socket_, INFO, command);
                std::cout << "Exiting..." << std::endl;
                LOG(INFO) << "Exiting...";
                Disconnect_();
                return;
                break;
            }
            default:
            {
                MessageObject::SendMessageObject(socket_, INFO, command);
                while(true){
                    MessageObject result = MessageObject::RecvMessageObject(socket_);
                    std::cout << result.getMessage() << std::endl;
                    if (result.getSignature() == SUCCESS){
                        break;
                    };
                }
                break;
            }
        }
    }
    catch(...){
        return;
    }
    return;
}

inline void ClientClass::Disconnect_(){
    this->~ClientClass();
    return;
}