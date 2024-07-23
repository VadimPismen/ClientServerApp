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
                    MessageObject::SendMessageObject(socket_, BAD);
                    isOnline_ = false;
                    parent_->DeleteClient(socket_, IP_ + ":" + std::to_string(port_) + ": couldn't login. Kicked!");
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
        if (isOnline_){
            LostConnection_();
        }
        return;
    }
}

void ClientThread::DisconnectClient_()
{
    isOnline_ = false;
    parent_->DeleteClient(socket_, name_ + "(" + IP_ + ":" + std::to_string(port_) + ") left.");
    return;
}

void ClientThread::LostConnection_()
{
    isOnline_ = false;
    parent_->DeleteClient(socket_, "lost connection to " + IP_ + ":" + std::to_string(port_) + ")");
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
                        clientCD_ = absNewPath;
                        if (access(absNewPath.c_str(), W_OK) == 0){
                            canWrite_ = true;
                            LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") entered " << clientCD_;
                        }
                        else{
                            canWrite_ = false;
                            MessageObject::SendMessageObject(socket_, INFO, "You can only read.");
                            LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") entered (readonly) " << clientCD_;
                        }
                        MessageObject::SendMessageObject(socket_, INFO, "Current path: " + absNewPath);
                    }
                    else{
                        MessageObject::SendMessageObject(socket_, INFO, NOACCESSTO + DIRECTORY);
                        LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") tried to enter " << absNewPath;
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
                LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") saved his new directory: " << clientCD_;
                break;
            }
            case Commands::GETDIRLIST:
            {
                std::string directory;
                std::string strArgs;
                std::bitset<4> args;
                std::vector<std::string> names;
                std::vector<std::string> dirs;
                std::vector<std::string> others;
                try{
                    strArgs = command.substr(3);
                    while (strArgs[0] == ' '){
                        strArgs.erase(0, 1);
                    }
                    if (!strArgs.empty()){
                        size_t endOfArgs = std::string::npos;
                        if (strArgs[0] == '-'){
                            for(size_t i = 0; i < strArgs.length(); i++){
                                if (strArgs[i] == ' '){
                                    endOfArgs = i;
                                    break;
                                }
                                switch(strArgs[i])
                                {
                                    case 'a':
                                        args.set(1);
                                        break;
                                    case 'D':
                                        args.set(2);
                                        break;
                                    case 'F':
                                        args.set(3);
                                        break;
                                }
                            }
                            if (endOfArgs != std::string::npos){
                                directory = strArgs.substr(endOfArgs);
                                while (directory[0] == ' '){
                                    directory.erase(0, 1);
                                }
                                if (!directory.empty()){
                                    args.set(0);
                                }
                            }
                        }
                        else if (strArgs[0] == '\"'){
                            size_t endOfDirectory = strArgs.find('\"', 1);
                            if (endOfDirectory == std::string::npos)
                            {
                                MessageObject::SendMessageObject(socket_, INFO, "Where is the second \" ?");
                                MessageObject::SendMessageObject(socket_, SUCCESS);
                                break;
                            }
                            directory = strArgs.substr(0, endOfDirectory);
                            directory.erase(0, 1);
                            args.set(0);
                        }
                        else{
                            directory = strArgs;
                            args.set(0);
                        }
                    }
                }
                catch(...){}
                if (args[0]){
                    try{
                        directory = GetAbsolutePath_(directory);
                    }
                    catch(...){
                        MessageObject::SendMessageObject(socket_, INFO, "No such directory.");
                        MessageObject::SendMessageObject(socket_, SUCCESS);
                        break;
                    }
                }
                else{
                    directory = clientCD_;
                }
                DIR *dir = opendir (directory.c_str());
                struct dirent *ent;
                std::string fileName;
                if (dir != nullptr) {
                    while ((ent = readdir (dir)) != NULL) {
                        fileName = ent->d_name;
                        unsigned char fileType = ent->d_type;
                        if (fileName[0] == '.' && !args[1]){
                            continue;
                        }
                        if (fileType == DT_DIR){
                            if (!args[2]){
                                dirs.push_back(fileName);
                                continue;
                            }
                        }
                        else if (fileType == DT_REG){
                            if (!args[3]){
                                names.push_back(fileName);
                            }
                        }
                        else{
                            others.push_back(fileName);
                        }
                    }
                    closedir (dir);
                    std::sort(names.begin(), names.end());
                    if (!args[2]){
                        for(std::string name : dirs){
                            MessageObject::SendMessageObject(socket_, INFO, "*dir* " + name);
                        }
                    }
                    if (!args[3]){
                        for(std::string name : names){
                            MessageObject::SendMessageObject(socket_, INFO, name);
                        }
                    }
                    if (!others.empty()){
                        for(std::string name : others){
                            MessageObject::SendMessageObject(socket_, INFO, "*smthg* " + name);
                        }
                    }
                    LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") got a list of files in " << directory;
                }
                else {
                    MessageObject::SendMessageObject(socket_, INFO, "Current directory is not available!");
                    LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") tried to get a list of files in " << directory;
                }
                MessageObject::SendMessageObject(socket_, SUCCESS);
                break;
            }
            case Commands::PROCS:
            {
                std::bitset<2> args;
                try{
                    std::string strArgs = command.substr(6);
                    while (strArgs[0] == ' '){
                        strArgs.erase(0, 1);
                    }
                    if (!strArgs.empty()){
                        if (strArgs[0] == '-'){
                            for(size_t i = 0; i < strArgs.length(); i++){
                                if (strArgs[i] == ' '){
                                    break;
                                }
                                switch(strArgs[i])
                                {
                                    case 'a':
                                        args.set(0);
                                        break;
                                    case 'f':
                                        args.set(1);
                                        break;
                                }
                            }
                        }
                    }
                }
                catch(...){};
                std::string procDir;
                std::string procInfo;
                char fileBuf[FILEBLOCKSIZE];
                DIR *dir = opendir ("/proc");
                struct dirent *ent;
                std::string fileName;
                if (dir != nullptr) {
                    while ((ent = readdir(dir)) != NULL) {
                        std::string allProcInfo = "";
                        if (std::isdigit(ent->d_name[0])){
                            procDir = "/proc/" + std::string(ent->d_name);
                            if (access(procDir.c_str(), R_OK) == 0){
                                int statFile = open((procDir + "/stat").c_str(), O_RDONLY);
                                bool isSystem = false;
                                std::string exe;
                                ssize_t bytes = readlink((procDir + "/exe").c_str(), fileBuf, FILEBLOCKSIZE);
                                if (bytes > 0){
                                    exe = "\texe: " + std::string(fileBuf, bytes) + "\n";
                                }
                                else{
                                    if (args[0]){
                                        isSystem = true;
                                    }
                                    else{
                                        continue;
                                    }
                                }
                                if (statFile >= 0){
                                    std::stringstream ss;
                                    read(statFile, &fileBuf, FILEBLOCKSIZE);
                                    ss << fileBuf;
                                    std::getline(ss, procInfo, ' ');
                                    allProcInfo += procInfo + ":\n";
                                    
                                    std::getline(ss, procInfo, ')');
                                    procInfo.erase(0, 1);
                                    if (isSystem){
                                        procInfo = "[" + procInfo + "]";
                                    }
                                    allProcInfo += "\tName: " + procInfo + "\n";
                                    close(statFile);
                                }
                                else{
                                    continue;
                                }
                                if (!exe.empty()){
                                    allProcInfo += exe;
                                }
                                procInfo = "";
                                bytes = readlink((procDir + "/cwd").c_str(), fileBuf, FILEBLOCKSIZE);
                                if (bytes >= 0){
                                    allProcInfo += "\tcwd: " + std::string(fileBuf, bytes) + "\n";
                                }
                                if (!isSystem){
                                    int cmdFile = open((procDir + "/cmdline").c_str(), O_RDONLY);
                                    if (cmdFile >= 0){
                                        ssize_t bytes = 0;
                                        std::string cmd;
                                        std::stringstream ss;
                                        while (true){
                                            bytes = read(cmdFile, &fileBuf, FILEBLOCKSIZE);
                                            if (bytes > 0){
                                                ss << std::string(fileBuf, bytes);
                                            }
                                            else{
                                                break;
                                            }
                                        }
                                        close(cmdFile);
                                        allProcInfo += "\tcmdline: ";
                                        while (std::getline(ss, cmd, '\0')){
                                            allProcInfo += cmd + " ";
                                        }
                                        allProcInfo += "\n";
                                    }
                                }
                                if (args[1]){
                                    DIR *descrDir = opendir((procDir + "/fd").c_str());
                                    struct dirent *descrEnt;
                                    if (descrDir != nullptr) {
                                        while ((descrEnt = readdir(descrDir)) != NULL) {
                                            struct stat fileInfo;
                                            char modeval [9];
                                            procInfo = "\t";
                                            if(stat((procDir + "/fd/" + descrEnt->d_name).c_str(), &fileInfo) == 0){
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
                                            bytes = readlink((procDir + "/fd/" + descrEnt->d_name).c_str(), fileBuf, FILEBLOCKSIZE);
                                            if (bytes >= 0){
                                                allProcInfo += "\t" + std::string(modeval, 9) + " " + descrEnt->d_name + " -> " + std::string(fileBuf, bytes) + "\n";
                                            }
                                        }
                                        closedir(descrDir);
                                    }
                                }
                                MessageObject::SendMessageObject(socket_, INFO, allProcInfo);
                            }
                        }
                    }
                    closedir(dir);
                }
                MessageObject::SendMessageObject(socket_, SUCCESS);
                if (args[0]){
                    LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") got a list of server's system processes";
                }
                else{
                    LOG(INFO) << name_ << '(' << IP_ << ':' << port_ << ") got a list of server's processes";
                }
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
                                        LOG(INFO) << "file " << absPath.string() << " was loaded by " << name_ << '(' << IP_ << ':' << port_ << ")";
                                    }
                                    else if (sizeOfBlock == 0){
                                        MessageObject::SendMessageObject(socket_, SUCCESS);
                                        LOG(INFO) << "file " << absPath.string() << " was loaded by " << name_ << '(' << IP_ << ':' << port_ << ")";
                                    }
                                    else{
                                        MessageObject::SendMessageObject(socket_, BAD, "Something got wrong with file on server.\nDownload is interrupted!");
                                        LOG(WARNING) << "something got wrong with file " << absPath.string() << " Client's [" << name_ << '(' << IP_ << ':' << port_ << ")] download is interrupted!";
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
    catch(...){
        MessageObject::SendMessageObject(socket_, INFO, "Undefined command");
        MessageObject::SendMessageObject(socket_, SUCCESS);
    }
    return;
}

void ClientThread::interceptLoadFileInterruption_(bool &isNotInterrupted)
{
    if (MessageObject::RecvMessageObject(socket_).getSignature() != SUCCESS){
        isNotInterrupted = false;
        MessageObject::SendMessageObject(socket_, BAD);
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

