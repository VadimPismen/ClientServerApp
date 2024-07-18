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
                    MessageObject::SendMessageObject(socket_, INFO, "Current path: " + dir);
                    MessageObject::SendMessageObject(socket_, SUCCESS);
                }
                else{
                    MessageObject::SendMessageObject(socket_, INFO, "Old path " + dir + " is not available.");
                    MessageObject::SendMessageObject(socket_, INFO, "Current path: " + clientCD_);
                    MessageObject::SendMessageObject(socket_, SUCCESS);
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
                        MessageObject::SendMessageObject(socket_, INFO, HELPSTRINGS.at(COMMANDS.at(secondArg)));
                    }
                    catch(...)
                    {
                        MessageObject::SendMessageObject(socket_, INFO, UNDEFCOM + secondArg);
                    }
                }
                else{
                    for(auto& helpStr : HELPSTRINGS){
                        MessageObject::SendMessageObject(socket_, INFO, helpStr.second);
                    }
                }
                MessageObject::SendMessageObject(socket_, SUCCESS);
                break;
            }
            case Commands::CD:
            {
                std::string secondArg;
                try{
                    secondArg = command.substr(3);
                }
                catch(...){
                    MessageObject::SendMessageObject(socket_, INFO, clientCD_);
                    MessageObject::SendMessageObject(socket_, SUCCESS);
                    break;
                }
                try{
                    std::string absNewPath = GetAbsolutePath_(secondArg);
                    boost::filesystem::path path(absNewPath);
                    if (!boost::filesystem::exists(path)){
                        MessageObject::SendMessageObject(socket_, INFO, "This directory does not exist!");
                        MessageObject::SendMessageObject(socket_, SUCCESS);
                        break;
                    }
                    if (!boost::filesystem::is_directory(path)){
                        MessageObject::SendMessageObject(socket_, INFO, "This is not a directory!");
                        MessageObject::SendMessageObject(socket_, SUCCESS);
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
                        MessageObject::SendMessageObject(socket_, INFO, "Current path: " + absNewPath);
                    }
                    else{
                        MessageObject::SendMessageObject(socket_, INFO, NOACCESSTO + DIRECTORY);
                    }
                }
                catch(...){
                    MessageObject::SendMessageObject(socket_, INFO, "This directory does not exist.");
                }
                MessageObject::SendMessageObject(socket_, SUCCESS);
                break;
            }
            case Commands::SAVEDIR:
            {
                parent_->SaveUserDir(name_, clientCD_);
                MessageObject::SendMessageObject(socket_, INFO, "Saved directory: " + clientCD_);
                MessageObject::SendMessageObject(socket_, SUCCESS);
                break;
            }
            case Commands::GETDIRLIST:
            {
                std::string secondArg;
                try{
                    secondArg = command.substr(3);
                }
                catch(...){
                    DIR *dir = opendir (clientCD_.c_str());
                    struct dirent *ent;
                    if (dir != nullptr) {
                        while ((ent = readdir (dir)) != NULL) {
                            MessageObject::SendMessageObject(socket_, INFO, ent->d_name);
                        }
                        closedir(dir);
                    }
                    else {
                        MessageObject::SendMessageObject(socket_, INFO, "Current directory is not available!");
                    }
                    MessageObject::SendMessageObject(socket_, SUCCESS);
                    break;
                }
                try{
                    std::string absNewPath = GetAbsolutePath_(secondArg);
                    DIR *dir = opendir (absNewPath.c_str());
                    struct dirent *ent;
                    if (dir != nullptr) {
                        while ((ent = readdir (dir)) != NULL) {
                            MessageObject::SendMessageObject(socket_, INFO, ent->d_name);
                        }
                        closedir (dir);
                    }
                    else {
                        MessageObject::SendMessageObject(socket_, INFO, "Current directory is not available!");
                    }
                }
                catch(...){
                    MessageObject::SendMessageObject(socket_, INFO, "This directory does not exist.");
                }
                MessageObject::SendMessageObject(socket_, SUCCESS);
                break;
            }
            case Commands::PROCS:
            {
                std::ifstream file;
                std::string procDir;
                std::string procInfo;
                for (auto const& dir_entry : std::filesystem::directory_iterator{"/proc"}){
                    std::string allProcInfo = "";
                    if (std::isdigit(dir_entry.path().filename().string()[0])){
                        procDir = dir_entry.path().string();
                        if (access(procDir.c_str(), R_OK) == 0){
                            file.open(procDir + "/stat");
                            if (file.is_open()){
                                
                                std::getline(file, procInfo, ' ');
                                allProcInfo += procInfo + ":\n";

                                std::getline(file, procInfo, ')');
                                procInfo.erase(0, 1);
                                allProcInfo += "\tName: " + procInfo + "\n";
                                file.close();
                            }
                            else{
                                continue;
                            }
                            char buf[FILEBLOCKSIZE];
                            procInfo = "";
                            ssize_t bytes = readlink((procDir + "/exe").c_str(), buf, FILEBLOCKSIZE);
                            if (bytes > 0){
                                allProcInfo += "\texe: " + std::string(buf, bytes) + "\n";
                            }

                            bytes = readlink((procDir + "/cwd").c_str(), buf, FILEBLOCKSIZE);
                            if (bytes >= 0){
                                allProcInfo += "\tcwd: " + std::string(buf, bytes) + "\n";
                            }
                            file.open(procDir + "/cmdline");
                            if (file.is_open()){
                                file.read(buf, FILEBLOCKSIZE);
                                if (file.gcount()){
                                    allProcInfo += "\tcmdline: " + std::string(buf, file.gcount()) + "\n";
                                }
                                file.close();
                            }
                            if (access((procDir + "/fd").c_str(), R_OK) == 0){
                                for (auto const& descriptor : std::filesystem::directory_iterator{procDir + "/fd"}){
                                    struct stat fileInfo;
                                    char modeval [9];
                                    procInfo = "\t\t";
                                    if(stat(descriptor.path().c_str(), &fileInfo) == 0){
                                        mode_t perm = fileInfo.st_mode;
                                        modeval[0] = (perm & S_IRUSR) ? 'r' : '-';
                                        modeval[1] = (perm & S_IWUSR) ? 'w' : '-';
                                        modeval[2] = (perm & S_IXUSR) ? 'x' : '-';
                                        modeval[3] = (perm & S_IRGRP) ? 'r' : '-';
                                        modeval[4] = (perm & S_IWGRP) ? 'w' : '-';
                                        modeval[5] = (perm & S_IXGRP) ? 'x' : '-';
                                        modeval[6] = (perm & S_IROTH) ? 'r' : '-';
                                        modeval[7] = (perm & S_IWOTH) ? 'w' : '-';
                                        modeval[8] = (perm & S_IXOTH) ? 'x' : '-';
                                    }
                                    bytes = readlink((descriptor.path()).c_str(), buf, FILEBLOCKSIZE);
                                    if (bytes >= 0){
                                        allProcInfo += "\t\t" + std::string(modeval, 9) + " " + descriptor.path().filename().string() + " -> " + std::string(buf, bytes) + "\n";
                                    }
                                }
                            }
                            MessageObject::SendMessageObject(socket_, INFO, allProcInfo);
                        }
                    }
                }
                MessageObject::SendMessageObject(socket_, SUCCESS);
                break;
            }
            case Commands::LOADFILE:
            {
                std::string serverFileAdr = command.substr(firstArg.length() + 1);
                try{
                    boost::filesystem::path relPath(serverFileAdr);
                    boost::filesystem::path absPath = boost::filesystem::canonical(relPath, boost::filesystem::path(clientCD_));
                    int file = open(absPath.string().c_str(), O_RDONLY);
                    if (file >= 0){
                        MessageObject::SendMessageObject(socket_, SUCCESS);
                        char buf[FILEBLOCKSIZE];
                        ssize_t sizeOfBlock = 0;
                        bool isNotInterrupted = true;
                        badLoadThread_ = boost::thread(&ClientThread::interceptLoadFileInterruption_, this, isNotInterrupted);
                        while(true){
                            if (isNotInterrupted){
                                sizeOfBlock = read(file, buf, FILEBLOCKSIZE);
                                if (sizeOfBlock < FILEBLOCKSIZE){
                                    if (sizeOfBlock > 0){
                                        MessageObject::SendMessageObject(socket_, LOAD, buf, sizeOfBlock);
                                        MessageObject::SendMessageObject(socket_, SUCCESS);
                                    }
                                    else if (sizeOfBlock == 0){
                                        MessageObject::SendMessageObject(socket_, SUCCESS);
                                    }
                                    else{
                                        MessageObject::SendMessageObject(socket_, BADLOAD, "Something got wrong with file on server.\nDownload is interrupted!");
                                    }
                                    badLoadThread_.join();
                                    break;
                                }
                                MessageObject::SendMessageObject(socket_, LOAD, buf, FILEBLOCKSIZE);
                            }
                            else{
                                break;
                            }
                        }
                        close(file);
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
                MessageObject::SendMessageObject(socket_, INFO, "Undefined command");
                MessageObject::SendMessageObject(socket_, SUCCESS);
                break;
            }
        }
    }
    catch(std::string error_message){
        MessageObject::SendMessageObject(socket_, INFO, error_message);
        MessageObject::SendMessageObject(socket_, SUCCESS);
    }
    return;
}

void ClientThread::interceptLoadFileInterruption_(bool &isNotInterrupted)
{
    if (MessageObject::RecvMessageObject(socket_).getSignature() != SUCCESS){
        isNotInterrupted = false;
        MessageObject::SendMessageObject(socket_, BADLOAD);
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

