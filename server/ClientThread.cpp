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
        {
        unsigned char attempts = 3;
        std::string login;
        std::string password;
        while(true){
            login = MessageObject::RecvMessageObject(socket_).getMessage();
            password = MessageObject::RecvMessageObject(socket_).getMessage();
            if (parent_->LookForAccount(login, password)){
                name_ = login;
                MessageObject::SendMessageObject(socket_, SUCCESS);
                LOG(INFO) << IP_ << ':' << port_ << " is " << name_;
                std::string dir = parent_->GetUserDirFromConfs(name_);
                if (access(dir.c_str(), R_OK) == 0){
                    if (access(dir.c_str(), W_OK) == 0){
                        canWrite_ = true;
                    }
                    else{
                        canWrite_ = false;
                        MessageObject::SendMessageObject(socket_, INFO, "You can only read.");
                    }
                    clientCD_ = dir;
                    MessageObject::SendMessageObject(socket_, SUCCESS, "Current path: " + dir);
                }
                else{
                    MessageObject::SendMessageObject(socket_, INFO, "Old path " + dir + " is not available.");
                    MessageObject::SendMessageObject(socket_, SUCCESS, "Current path: " + clientCD_);
                }
                break;
            }
            else{
                attempts--;
                if (attempts){
                    MessageObject::SendMessageObject(socket_, INFO, std::to_string(attempts) + " attempts left.");
                    LOG(INFO) << IP_ << ':' << port_ << ": unsuccessful login attempt.";
                }
                else{
                    MessageObject::SendMessageObject(socket_, GETOUT);
                    LOG(WARNING) << IP_ << ':' << port_ << ": couldn't login. Kicked!";
                    parent_->DeleteClient(socket_);
                    return;
                }
            }
        }
        }
        while(true){
            ParseCommand_(MessageObject::RecvMessageObject(socket_).getMessage());
        }
    }
    catch(ConnectionLostException){
        DisconnectClient_();
        return;
    }
}

void ClientThread::DisconnectClient_()
{
    LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") left.";
    parent_->DeleteClient(socket_);
    return;
}

void ClientThread::LostConnection_()
{
    LOG(WARNING) << "lost connection to " << IP_ << ':' << port_;
    parent_->DeleteClient(socket_);
    return;
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
                DisconnectClient_();
                break;
            }
            case Commands::HELP:
            {
                std::string secondArg;
                ss >> secondArg;
                if (!secondArg.empty()){
                    try{
                        MessageObject::SendMessageObject(socket_, SUCCESS, HELPSTRINGS.at(COMMANDS.at(secondArg)));
                    }
                    catch(...)
                    {
                        MessageObject::SendMessageObject(socket_, SUCCESS, UNDEFCOM + secondArg);
                    }
                }
                else{
                    for(auto& helpStr : HELPSTRINGS){
                        MessageObject::SendMessageObject(socket_, INFO, helpStr.second);
                    }
                    MessageObject::SendMessageObject(socket_, SUCCESS);
                }
                break;
            }
            case Commands::CD:
            {
                std::string secondArg;
                try{
                    secondArg = command.substr(3);
                }
                catch(...){
                    MessageObject::SendMessageObject(socket_, SUCCESS, clientCD_);
                    break;
                }
                try{
                    std::string absNewPath = GetAbsolutePath_(secondArg);
                    boost::filesystem::path path(absNewPath);
                    if (!boost::filesystem::exists(path)){
                        MessageObject::SendMessageObject(socket_, SUCCESS, "This directory doesnt exist!");
                        break;
                    }
                    if (!boost::filesystem::is_directory(path)){
                        MessageObject::SendMessageObject(socket_, SUCCESS, "This is not a directory!");
                        break;
                    }
                    std::string result = "";
                    if (access(absNewPath.c_str(), R_OK) == 0){
                        if (access(absNewPath.c_str(), W_OK) == 0){
                            canWrite_ = true;
                        }
                        else{
                            canWrite_ = false;
                            MessageObject::SendMessageObject(socket_, INFO, "You can only read.");
                        }
                        clientCD_ = absNewPath;
                        MessageObject::SendMessageObject(socket_, SUCCESS, "Current path: " + absNewPath);
                    }
                    else{
                        MessageObject::SendMessageObject(socket_, SUCCESS, NOACCESSTO + DIRECTORY);
                        break;
                    }
                }
                catch(...){
                    MessageObject::SendMessageObject(socket_, SUCCESS, "This directory does not exist.");
                    break;
                }
                break;
            }
            case Commands::SAVEDIR:
            {
                parent_->SaveUserDir(name_, clientCD_);
                MessageObject::SendMessageObject(socket_, SUCCESS, "Saved directory: " + clientCD_);
                break;
            }
            case Commands::LOADFILE:
            {
                std::string serverFileAdr = command.substr(firstArg.length() + 1);
                try{
                    boost::filesystem::path relPath(serverFileAdr);
                    boost::filesystem::path absPath = boost::filesystem::canonical(relPath, boost::filesystem::path(clientCD_));
                    std::ifstream file(absPath.string(), std::ios_base::binary);
                    if (file.is_open()){
                        MessageObject::SendMessageObject(socket_, SUCCESS);
                        char buf[FILEBLOCKSIZE];
                        size_t sizeOfBlock = 0;
                        while(true){
                            file.read(buf, FILEBLOCKSIZE);
                            sizeOfBlock = file.gcount();
                            if (sizeOfBlock < FILEBLOCKSIZE){
                                if (sizeOfBlock == 0){
                                    MessageObject::SendMessageObject(socket_, SUCCESS);
                                }
                                else{
                                    std::vector<char> bytes = {};
                                    std::copy(buf, &buf[sizeOfBlock], back_inserter(bytes));
                                    MessageObject::SendMessageObject(socket_, LOAD, bytes);
                                    MessageObject::SendMessageObject(socket_, SUCCESS);
                                }
                                break;
                            }
                            std::vector<char> bytes = {};
                            std::copy(buf, &buf[sizeOfBlock], back_inserter(bytes));
                            MessageObject::SendMessageObject(socket_, LOAD, bytes);
                        }
                        file.close();
                        break;
                    }
                    else{
                        MessageObject::SendMessageObject(socket_, INFO, "This is not a file.");
                        break;
                    }
                }
                catch(...){
                    MessageObject::SendMessageObject(socket_, INFO, "File does not exist.");
                    break;
                }
                break;
            }
            default:
            {
                MessageObject::SendMessageObject(socket_, SUCCESS, (ExecuteSystemCommandAndGetResult_(command)));
                break;
            }
        }
    }
    catch(...){
        MessageObject::SendMessageObject(socket_, SUCCESS, (ExecuteSystemCommandAndGetResult_(command)));
    }
    return;
}

std::string ClientThread::ExecuteSystemCommandAndGetResult_(const std::string command){
    std::string result = "";
    FILE* pipe = popen(("cd " + clientCD_ + "; " + command).c_str(), "r");
    if (!pipe){
        return "What?";
    }
    char buffer[BUFFSIZE];
    try {
        while (fgets(buffer, BUFFSIZE, pipe) != NULL) {
            result += buffer;
        }
    } catch (...) {
        pclose(pipe);
        return "What?";
    }
    pclose(pipe);
    return result;
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

