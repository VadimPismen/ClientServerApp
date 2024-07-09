#include "ServerClass.h"

using namespace CSA;

ServerClass::ServerClass(std::string cfgFile): cfgFile_(cfgFile){
    cfg_.readFile("config.cfg");
    libconfig::Setting& root_ =  cfg_.getRoot();
    libconfig::Setting& serverconfs_ = root_.lookup("server");
    serverconfs_.lookupValue("port", port_);
}

ServerClass::~ServerClass(){
    close(serverSocket_);
    adminCommandsThread_.interrupt();
}

void ServerClass::OpenServer(){
    serverSocket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket_ < 0)
    {
        LOG(ERROR) << "socket creation error!";
        perror("Socket creation error: ");
        exit(1);
    }

    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port_);
    addr_.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(serverSocket_, (struct sockaddr*)&addr_, sizeof(addr_)) < 0)
    {
        LOG(ERROR) << "socket binding error!";
        perror("Bind error: ");
        exit(1);
    }
    std::cout << "Server is opened on " << inet_ntoa(addr_.sin_addr) << ':' << port_ << std::endl;
    LOG(INFO) << "Server is opened on " << inet_ntoa(addr_.sin_addr) << ':' << port_;
    adminCommandsThread_ = boost::thread(&ServerClass::GetAdminCommands_, this);
    while(true){
        listen(serverSocket_, SOMAXCONN);
        int sock = accept(serverSocket_, NULL, NULL);
        if (sock >= 0){
            listOfClients_.try_emplace(sock);
            listOfClients_[sock].StartWorking(sock, this);
        }
    }
}

void ServerClass::GetAdminCommands_(){
    try{
        while (true){
            boost::this_thread::interruption_point();
            std::string command;
            std::getline(std::cin, command);
            if (command == "users"){
                for(auto& user : listOfClients_){
                    std::cout << user.first << " " << user.second.getIP() << " " << user.second.getName() << std::endl;
                }
                continue;
            }
            if (command == "stop"){
                this->~ServerClass();
                exit(0);
                return;
            }
        }
    }
    catch(boost::thread_interrupted){
        return;
    }
}

void ServerClass::DeleteClient(int socket){
    close(socket);
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
