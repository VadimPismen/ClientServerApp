#include "ClientThread.h"

ClientThread::ClientThread(int socket): socket_(socket){};

ClientThread::~ClientThread(){
    close(socket_);
};

void ClientThread::WorkWithClient(){
    try{
        while(true){
        recv(socket_, &buffer, 512, 0);
        send(socket_, &buffer, 512, 0);
        }
    }
    catch(...){
        this->~ClientThread();
        return;
    }
}