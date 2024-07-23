#include "ServerClass.h"

using namespace CSA;

ServerClass::ServerClass(std::string cfgFile): cfgFile_(cfgFile){
    cfg_.readFile("config.cfg");
    libconfig::Setting& root_ =  cfg_.getRoot();
    libconfig::Setting& serverconfs_ = root_.lookup("server");
    serverconfs_.lookupValue("port", port_);

    serverconfs_.lookupValue("logs", logsDir_);
    logsDir_ = GetAbsolutePath_(logsDir_, std::filesystem::current_path().string());
    FLAGS_log_dir = logsDir_;
    FLAGS_alsologtostderr = true;
    google::InitGoogleLogging("Server");
}

ServerClass::~ServerClass(){}

void ServerClass::OpenServer(){
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        throw SocketCreationException();
    }
    
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "socket binding error!";
        throw BindException();
    }
    LOG(INFO) << "Server is opened on port " << port_;
    adminCommandsThread_ = boost::thread(&ServerClass::GetAdminCommands_, this);
    while(isOnline_){
        listen(serverSocket_, SOMAXCONN);
        int sock = accept(serverSocket_, NULL, NULL);
        if (sock >= 0){
            listOfClients_.try_emplace(sock);
            listOfClients_[sock].StartWorking(sock, this);
        }
        else{
            break;
        }
    }
}

void ServerClass::GetAdminCommands_(){
    while (true){
        std::string command;
        std::getline(std::cin, command);
        if (command == "users"){
            for(auto& user : listOfClients_){
                std::cout << user.first << " " << user.second.getIP() << " " << user.second.getName() << std::endl;
            }
            continue;
        }
        if (command == "stop"){
            isOnline_ = false;
            close(serverSocket_);
            exit(0);
            return;
        }
    }
}

void ServerClass::DeleteClient(int socket, std::string message){
    close(socket);
    LOG(INFO) << message;
    listOfClients_.erase(socket);
    return;
}

bool ServerClass::LookForAccount(std::string login, std::string password){
    libconfig::Setting& root_ =  cfg_.getRoot();
    libconfig::Setting& accounts_ = root_.lookup("accounts");
    int count = accounts_.getLength();
    std::string checkLogin;
    std::string checkPassword;
    for (int i = 0; i < count; i++){
        const libconfig::Setting& account = accounts_[i];
        std::string checkLogin;
        account.lookupValue("login", checkLogin);
        std::string checkPassword;
        account.lookupValue("password", checkPassword);
        if (login == checkLogin && password == checkPassword)
        {
            return true;
        }
    }
    return false;
}

std::string ServerClass::GetUserDirFromConfs(std::string login){
    libconfig::Setting& root_ =  cfg_.getRoot();
    libconfig::Setting& accounts_ = root_.lookup("accounts");
    int count = accounts_.getLength();
    for (int i = 0; i < count; i++){
        const libconfig::Setting& account = accounts_[i];
        std::string checkLogin;
        account.lookupValue("login", checkLogin);
        if (login == checkLogin)
        {
            std::string dir;
            account.lookupValue("dir", dir);
            return dir;
        }
    }
    return "";
}

void ServerClass::SaveUserDir(std::string login, std::string dir)
{
    libconfig::Setting& root_ =  cfg_.getRoot();
    libconfig::Setting& accounts_ = root_.lookup("accounts");
    int count = accounts_.getLength();
    for (int i = 0; i < count; i++){
        const libconfig::Setting& account = accounts_[i];
        std::string checkLogin;
        account.lookupValue("login", checkLogin);
        if (login == checkLogin)
        {
            libconfig::Setting& dirSet = account.lookup("dir");
            dirSet = dir;
            cfg_.writeFile(cfgFile_);
        }
    }
}

std::string ServerClass::GetAbsolutePath_(const std::string path, const std::string base){
    boost::filesystem::path newPath(path);
    boost::filesystem::path basePath(base);
    if (newPath.is_absolute()){
        return path;
    }
    else{
        return boost::filesystem::canonical(newPath, basePath).string();
    }
}