#include "ClientClass.h"

using namespace CSA;

ClientClass::ClientClass(std::string cfgFile): cfgFile_(cfgFile){
    cfg_.readFile(cfgFile);
    libconfig::Setting& root_ =  cfg_.getRoot();

    root_.lookupValue("logs", logsDir_);
    logsDir_ = GetAbsolutePath_(logsDir_, std::filesystem::current_path().string());
    FLAGS_log_dir = logsDir_;
    google::InitGoogleLogging("Client");

    libconfig::Setting& newLoadDirSet =  root_.lookup("loadDir");
    std::string newLoadDir = GetAbsolutePath_(newLoadDirSet, std::filesystem::current_path().string());
    if (std::filesystem::is_directory(newLoadDir)){
        loadDir_ = newLoadDir;
    }
    else{
        newLoadDirSet = loadDir_;
        cfg_.writeFile(cfgFile);
        std::cout << "Old download directory " + newLoadDir + " is not available." << std::endl;
    }
    std::cout << "Current download directory is " << loadDir_ << std::endl;
    LOG(INFO) << "client object was created. Download directory is " << loadDir_;
};

ClientClass::~ClientClass(){}

void ClientClass::StartConnection() {
    libconfig::Setting& connectionConfs_ = cfg_.getRoot().lookup("connection");
    connectionConfs_.lookupValue("ip", IP_);
    connectionConfs_.lookupValue("port", port_);

    socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        throw SocketCreationException();
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = inet_addr(IP_.c_str());
    if (connect(socket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "connection to " << IP_ << ':' << port_ << " is failed!";
        throw BindException();
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
                LOG(INFO) << "successful login as " << login_;
                while(true){
                    MessageObject result = MessageObject::RecvMessageObject(socket_);
                    if (result.getSignature() == SUCCESS){
                        break;
                    };
                    std::cout << result.getMessage() << std::endl;
                }
                break;
            }
            else{
                if (recv.getSignature() != BAD){
                    std::cout << recv.getMessage() << std::endl << std::endl;
                    LOG(INFO) << "unsuccessful attempt.";
                }
                else{
                    throw KickedException();
                }
            }
        }
        while(isOnline_){
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
        std::cout << "Alas!" << std::endl;
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
                std::string filePath = loadDir_ + '/' + clientFileName;
                int clientFile = open(filePath.c_str(), O_WRONLY | O_CREAT, S_IRWXU);
                LOG(INFO) << "try to load " << filePath;
                if (clientFile >= 0){
                    MessageObject::SendMessageObject(socket_, INFO, firstArg + " " + serverFileAdr);
                    MessageObject result = MessageObject::RecvMessageObject(socket_);
                    if (result.getSignature() == SUCCESS){
                        while(true){
                            result = MessageObject::RecvMessageObject(socket_);
                            char signature = result.getSignature();
                            if (signature == LOAD){
                                ssize_t writtenBytes = write(clientFile, result.getBytes().data(), result.getsizeOfMessage());
                                if (writtenBytes < 0){
                                    std::cout << "Something got wrong with client's file.\nDownload is interrupted!" << std::endl;
                                    LOG(WARNING) << "something got wrong with file. Download is interrupted!";
                                    MessageObject::SendMessageObject(socket_, BAD);
                                    close(clientFile);
                                    while (true){
                                        if (MessageObject::RecvMessageObject(socket_).getSignature() == BAD){
                                            break;
                                        }
                                    }
                                    break;
                                }
                            }
                            else if (signature == SUCCESS){
                                std::cout << "File was loaded to " << filePath << std::endl;
                                LOG(INFO) << "file is loaded.";
                                MessageObject::SendMessageObject(socket_, SUCCESS);
                                close(clientFile);
                                break;
                            }
                            else{
                                std::string message = result.getMessage();
                                std::cout << message << std::endl;
                                LOG(WARNING) << message;
                                close(clientFile);
                                MessageObject::SendMessageObject(socket_, SUCCESS);
                                break;
                            }
                        }
                    }
                    else{
                        std::cout << result.getMessage() << std::endl;
                        remove(filePath.c_str());
                        close(clientFile);
                        break;
                    }
                }
                else{
                    std::cout << "Cannot create/rewrite file!" << std::endl;
                    LOG(WARNING) << "cannot create/rewrite file!";
                }
                break;
            }
            case Commands::EXIT:
            {
                MessageObject::SendMessageObject(socket_, INFO, command);
                std::cout << "Exiting..." << std::endl;
                LOG(INFO) << "exiting...";
                Disconnect_();
                break;
            }
            case Commands::CLEARLOGS:
            {
                try{
                    std::filesystem::remove_all(logsDir_);
                    std::filesystem::create_directory(logsDir_);
                }
                catch(...){
                    std::cout << "Something got wrong..." << std::endl;
                    LOG(WARNING) << "clearing logs is unsuccessful.";
                    break;
                }
                std::cout << "Logs are cleared." << std::endl;
                LOG(INFO) << "logs was cleared.";
                break;
            }
            case Commands::LOADDIR:
            {
                std::string secondArg;
                try{
                    secondArg = command.substr(7);
                    while(secondArg[0] == ' '){
                        secondArg.erase(0, 1);
                    }
                }
                catch(...){
                    std::cout << "Current download directory is " << loadDir_ << std::endl;
                    LOG(INFO) << "current download directory is " << loadDir_;
                    break;
                }
                if (secondArg.empty()){
                    std::cout << "Current download directory is " << loadDir_ << std::endl;
                    LOG(INFO) << "current download directory is " << loadDir_;
                    break;
                }
                std::string absNewPath = GetAbsolutePath_(secondArg, loadDir_);
                boost::filesystem::path path(absNewPath);
                if (!boost::filesystem::exists(path)){
                    std::cout << "This directory does not exist!" << std::endl;
                    break;
                }
                if (!boost::filesystem::is_directory(path)){
                    std::cout << "This is not a directory!" << std::endl;
                    break;
                }
                if (access(absNewPath.c_str(), W_OK) == 0){
                    loadDir_ = absNewPath;
                    try{
                        libconfig::Setting& root_ =  cfg_.getRoot();
                        libconfig::Setting& newLoadDirSet =  root_.lookup("loadDir");
                        newLoadDirSet = loadDir_;
                        cfg_.writeFile("config.cfg");
                    }
                    catch(...){
                        std::cout << "Cannot save to configs." << std::endl;
                        LOG(WARNING) << "cannot save new download directory to configs!" << loadDir_;
                    }
                    std::cout << "Current download directory is " << loadDir_ << std::endl;
                    LOG(INFO) << "current download directory is " << loadDir_;
                }
                else{
                    std::cout << "Cannot write to " << absNewPath << std::endl;
                }
                break;
            }
            default:
            {
                MessageObject::SendMessageObject(socket_, INFO, command);
                while(true){
                    MessageObject result = MessageObject::RecvMessageObject(socket_);
                    if (result.getSignature() == SUCCESS){
                        break;
                    };
                    std::cout << result.getMessage() << std::endl;
                }
                break;
            }
        }
    }
    catch(...){
        std::cout << "Undefined command. Type \"help\" to get a list of possible commands." << std::endl;
        return;
    }
    return;
}

inline void ClientClass::Disconnect_(){
    close(socket_);
    isOnline_ = false;
    return;
}

std::string ClientClass::GetAbsolutePath_(const std::string path, const std::string base){
    boost::filesystem::path newPath(path);
    boost::filesystem::path basePath(base);
    if (newPath.is_absolute()){
        return path;
    }
    else{
        return boost::filesystem::canonical(newPath, basePath).string();
    }
}