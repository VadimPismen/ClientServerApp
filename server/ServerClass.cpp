#include "ServerClass.h"

using namespace CSA;

ServerClass::ServerClass(uint16_t port, std::string cfgFile): port_(port), cfgFile_(cfgFile){
    LookForAccount("Vadim", "hehe");
}

ServerClass::~ServerClass(){
    close(serverSocket_);
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
    LOG(INFO) << "Server is opened on " << inet_ntoa(addr_.sin_addr) << ':' << port_;
    while(true){
        listen(serverSocket_, SOMAXCONN);
        int sock = accept(serverSocket_, NULL, NULL);
        if (sock >= 0){
            listOfClients_.try_emplace(sock);
            listOfClients_[sock].StartWorking(sock, this);
        }
    }
}

void ServerClass::DeleteClient(int socket){
    close(socket);
    listOfClients_.erase(socket);
    return;
}

bool ServerClass::LookForAccount(std::string login, std::string password){
    libconfig::Config cfg_;
    cfg_.readFile("config.cfg");
    libconfig::Setting& root_ =  cfg_.getRoot();
    libconfig::Setting& accounts_ = root_.lookup("accounts");
    int count = accounts_.getLength();
    std::string checkLogin;
    std::string checkPassword;
    for (int i = 0; i < count; i++){
        const libconfig::Setting& account = accounts_[i];
        bool sos = accounts_.lookupValue("login", login);
        // Что-то не так
        return true;
    }
    return false;
}