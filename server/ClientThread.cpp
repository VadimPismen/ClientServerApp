#include "ClientThread.h"

using namespace CSA;

ClientThread::ClientThread(){}

ClientThread::~ClientThread(){}

void ClientThread::StartWorking(int socket, ServerClass* parent){
    socket_ = socket;
    parent_ = parent;

    struct sockaddr_in clientData;
    socklen_t size = sizeof(clientData);

    getsockname(socket, (struct sockaddr*)&clientData, &size);

    IP_ = inet_ntoa(clientData.sin_addr);
    port_ = htons(clientData.sin_port);

    LOG(INFO) << "new connection from " << IP_ << ':' << port_;
    workingThread_ = boost::thread(&ClientThread::WorkWithClient_, this);
    return;
}

void ClientThread::WorkWithClient_(){
    try{
        while(true){
            switch (state_)
            {
            case ClientState::LOGIN:
            {
                unsigned char attempts = 3;
                    while(true){
                    if (LogIn_()){
                        SendStringToClient_(SUCCESS);
                        LOG(INFO) << IP_ << ':' << port_ << " is " << name_;
                        std::string dir = parent_->GetUserDirFromConfs(name_);
                        if (access(dir.c_str(), R_OK) == 0){
                            if (access(dir.c_str(), W_OK) == 0){
                                canWrite_ = true;
                            }
                            else{
                                canWrite_ = false;
                                SendStringToClient_("You can only read.");
                            }
                            clientCD_ = dir;
                            SendStringToClient_("Current path: " + dir);
                        }
                        else{
                            SendStringToClient_("Old path " + dir + " is not available.");
                            SendStringToClient_("Current path: " + clientCD_);
                        }
                        SendStringToClient_(ENDOFRES);
                        state_ = ClientState::IDLE;
                        break;
                    }
                    else{
                        attempts--;
                        if (attempts){
                            SendStringToClient_(std::to_string(attempts) + " attempts left.");
                            LOG(INFO) << IP_ << ':' << port_ << ": unsuccessful login attempt.";
                        }
                        else{
                            SendStringToClient_(GETOUT);
                            LOG(WARNING) << IP_ << ':' << port_ << ": couldn't login. Kicked!";
                            parent_->DeleteClient(socket_);
                            return;
                        }
                    }
                }
                break;
            }
            case ClientState::EXIT:
            {
                LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") left.";
                parent_->DeleteClient(socket_);
                break;
            }
            case ClientState::IDLE:
            {
                ParseCommand_(GetStringFromClient_());
                break;
            }
            default:
                break;
            }
        }
    }
    catch(...){
        LOG(WARNING) << "lost connection to " << IP_ << ':' << port_;
        parent_->DeleteClient(socket_);
        return;
    }
}

bool ClientThread::LogIn_(){
    std::string login = GetStringFromClient_();
    std::string password = GetStringFromClient_();
    if (parent_->LookForAccount(login, password)){
        name_ = login;
        return true;
    }
    else{
        return false;
    }
}

void ClientThread::SendPieceToClient_(const std::string message){
    const char* buffer = message.c_str();
    if (send(socket_, buffer, CSA::BUFFSIZE, MSG_NOSIGNAL) < 0){
        throw ConnectionLostException();
    }
}

void ClientThread::SendStringToClient_(const std::string message){
    size_t sizeOfData = message.length();
    SendPieceToClient_(std::to_string(sizeOfData) + '\n');
    size_t countOfBlocks = sizeOfData / CSA::BUFFSIZE;
    if (sizeOfData % CSA::BUFFSIZE != 0){
        countOfBlocks++;
    }
    for (size_t i = 0; i < countOfBlocks; i++){
        SendPieceToClient_(message.substr(i*CSA::BUFFSIZE, CSA::BUFFSIZE));
    }
}

// void ClientThread::SendFileToClient_(std::ifstream file){
//     file.seekg(0, std::ios_base::end);
//     ssize_t sizeOfData = file.tellg();
//     file.seekg(0);
//     ssize_t countOfByteBlocks = sizeOfData / CSA::BUFFSIZE;
//     if (sizeOfData % CSA::BUFFSIZE != 0){
//         countOfByteBlocks++;
//     }
//     SendPieceToClient_(std::to_string(countOfByteBlocks) + '\n');
//     char buffer[BUFFSIZE];
//     for (ssize_t i = 0; i < countOfFileBlocks; i++){
//         file.read(buffer, BUFFSIZE);
        
//     }
// }

void ClientThread::SendBytesToClient_(char* buffer){
    if (send(socket_, buffer, CSA::BUFFSIZE, MSG_NOSIGNAL) < 0){
        throw ConnectionLostException();
    }
}

std::string ClientThread::GetStringFromClient_(){
    std::string signature = GetPieceFromClient_();
    if (signature != SIGN){
        LOG(WARNING) << IP_ << ':' << port_ << " kicked out as an outsider!";
        this->~ClientThread();
    }
    size_t sizeOfData;
    try{
        sizeOfData = std::stoull(GetPieceFromClient_());
    }
    catch(...){
        sizeOfData = 0;
    }
    std::string clientData = "";
    size_t countOfBlocks = sizeOfData / CSA::BUFFSIZE;
    if (sizeOfData % CSA::BUFFSIZE != 0){
        countOfBlocks++;
    }
    for (size_t i = 0; i < countOfBlocks; i++){
        clientData += GetPieceFromClient_();
    }
    return clientData;
}

std::string ClientThread::GetPieceFromClient_(){
    char buffer[CSA::BUFFSIZE];
    ssize_t countOfData = recv(socket_, &buffer, CSA::BUFFSIZE, MSG_NOSIGNAL);
        if (countOfData < 0){
            throw ConnectionLostException();
        }
    return std::string(buffer);
}

void ClientThread::ParseCommand_(const std::string command){
    std::string firstArg;
    std::istringstream ss(command);
    ss >> firstArg;
    try{
        const Commands com = COMMANDS.at(firstArg);
        switch(com)
        {
            case Commands::EXIT:
            {
                SendStringToClient_(GOODBYE);
                state_ = ClientState::EXIT;
                LOG(INFO) << IP_ << ':' << port_ << " disconnected.";
                break;
            }
            case Commands::HELP:
            {
                std::string secondArg;
                ss >> secondArg;
                if (!secondArg.empty()){
                    try{
                        SendStringToClient_(HELPSTRINGS.at(COMMANDS.at(secondArg)));
                    }
                    catch(...)
                    {
                        SendStringToClient_(UNDEFCOM + secondArg);
                    }
                }
                else{
                    std::string result;
                    for(auto& helpStr : HELPSTRINGS){
                        SendStringToClient_(helpStr.second);
                    }
                }
                SendStringToClient_(ENDOFRES);
                break;
            }
            case Commands::CD:
            {
                std::string secondArg;
                try{
                    secondArg = command.substr(3);
                }
                catch(...){
                    SendStringToClient_(clientCD_);
                    SendStringToClient_(ENDOFRES);
                    break;
                }
                try{
                    std::string absNewPath = GetAbsolutePath_(secondArg);
                    if (!boost::filesystem::is_directory(boost::filesystem::path(absNewPath))){
                        SendStringToClient_("This is not a directory!");
                        SendStringToClient_(ENDOFRES);
                        break;
                    }
                    if (access(absNewPath.c_str(), R_OK) == 0){
                        if (access(absNewPath.c_str(), W_OK) == 0){
                            canWrite_ = true;
                        }
                        else{
                            canWrite_ = false;
                            SendStringToClient_("You can only read.");
                        }
                        clientCD_ = absNewPath;
                        SendStringToClient_("Current path: " + absNewPath);
                        SendStringToClient_(ENDOFRES);
                    }
                    else{
                        SendStringToClient_(NOACCESSTO + DIRECTORY);
                        SendStringToClient_(ENDOFRES);
                        break;
                    }
                }
                catch(...){
                    SendStringToClient_("This directory does not exist.");
                    SendStringToClient_(ENDOFRES);
                    break;
                }
                break;
            }
            case Commands::SAVEDIR:
            {
                parent_->SaveUserDir(name_, clientCD_);
                SendStringToClient_("Saved directory: " + clientCD_);
                SendStringToClient_(ENDOFRES);
                break;
            }
            case Commands::LOADFILE:
            {
                size_t begOfFilePath = command.find('"');
                if (begOfFilePath == std::string::npos){
                    SendStringToClient_("Where is \"?");
                    SendStringToClient_(ENDOFRES);
                    break;
                }
                size_t endOfFilePath = command.find('"', begOfFilePath + 1) - 1;
                if (endOfFilePath == std::string::npos){
                    SendStringToClient_("Where is the second \"?");
                    SendStringToClient_(ENDOFRES);
                    break;
                }
                if (begOfFilePath < endOfFilePath){
                    std::string filePath = command.substr(begOfFilePath + 1, endOfFilePath - begOfFilePath);
                    std::ifstream file(GetAbsolutePath_(filePath), std::ios_base::binary);
                    if (!file.is_open()){
                        SendStringToClient_("Cannot open the file.");
                        SendStringToClient_(ENDOFRES);
                        break;
                    }
                    else{
                        std::string fileName = boost::filesystem::path(filePath).filename().string();
                        SendStringToClient_(fileName);
                        try{
                            std::string saveDir = command.substr(endOfFilePath + 2);
                            SendStringToClient_(saveDir);
                        }
                        catch(...){
                            SendStringToClient_("");
                        }
                        if (GetStringFromClient_() == SUCCESS){
                            file.seekg(0, std::ios_base::end);
                            ssize_t sizeOfData = file.tellg();
                            file.seekg(0);
                            ssize_t countOfByteBlocks = sizeOfData / CSA::BUFFSIZE;
                            ssize_t sizeOfByteTail = sizeOfData % CSA::BUFFSIZE;
                            if (sizeOfByteTail != 0){
                                countOfByteBlocks++;
                            }
                            SendPieceToClient_(std::to_string(countOfByteBlocks));
                            SendPieceToClient_(std::to_string(sizeOfByteTail));
                            char fileBuf[BUFFSIZE];
                            for (ssize_t i = 0; i < countOfByteBlocks; i++){
                                file.read(fileBuf, BUFFSIZE);
                                SendBytesToClient_(fileBuf);
                            }
                            file.close();
                            break;
                        }
                        else{
                            file.close();
                            break;
                        }
                    }
                }
                else{
                    SendStringToClient_("Empty file name.");
                    SendStringToClient_(ENDOFRES);
                    break;
                }
                break;
            }
            default:
            {
                SendStringToClient_(ExecuteSystemCommandAndGetResult_(command));
                SendStringToClient_(ENDOFRES);
                break;
            }
        }
    }
    catch(...){
        SendStringToClient_(ExecuteSystemCommandAndGetResult_(command));
        SendStringToClient_(ENDOFRES);
    }
    return;
}

std::string ClientThread::ExecuteSystemCommandAndGetResult_(const std::string command){
    std::string result = "";
    FILE* pipe = popen(("cd " + clientCD_ + "; " + command).c_str(), "r");
    if (!pipe){
        return BADRES;
    }
    char buffer[BUFFSIZE];
    try {
        while (fgets(buffer, BUFFSIZE, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        return BADRES;
    }
    pclose(pipe);
    return result;
}

std::string ClientThread::GetArgsFromClient_(size_t countOfArgs){
    std::string args;
    for (size_t i = 0; i < countOfArgs; i++){
        args += GetStringFromClient_();
    }
    return args;
}

std::string ClientThread::GetArgsFromClient_(){
    std::string args;
    size_t countOfArgs = std::stoull(GetStringFromClient_());
    for (size_t i = 0; i < countOfArgs; i++){
        args += GetStringFromClient_();
    }
    return args;
}

size_t ClientThread::GetCountOfArgsFromClient_(){
    return std::stoull(GetStringFromClient_());
}

std::string ClientThread::GetAbsolutePath_(std::string path){
    boost::filesystem::path newPath(path);
    boost::filesystem::path basePath(clientCD_);
    if (newPath.is_absolute()){
        return path;
    }
    else{
        return boost::filesystem::canonical(newPath, basePath).string();
    }
}

