// LinuxServer.cpp: определяет точку входа для приложения.
//

#include "LinuxServer.h"

using namespace std;

int main()
{
    int sock, listener;
    struct sockaddr_in addr;
    int bytes_read;

    listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listener, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("bind");
        exit(2);
    }

    listen(listener, 1);

    sock = accept(listener, NULL, NULL);
    if (sock < 0)
    {
        perror("accept");
        exit(3);
    }
    //char buf[1024];
    //bytes_read = recv(sock, buf, 1024, 0);
    //cout << buf;
    string message = "HTTP/1.1 200 OK\n\r"
        "Date: Wed, 11 Feb 2009 11 : 20 : 59 GMT\n\r"
        "Server : Apache\n\r"
        "Last - Modified : Wed, 11 Feb 2009 11 : 20 : 59 GMT\n\r"
        "Content - Language : ru\n\r"
        "Content - Type : text / html; charset = utf - 8\n\r"
        "Connection : close\n\r"
        "Content - Length: 82\n\r"
        "\n\r"
        "<html><body><a href=\"http://example.ru\">Example Corp.</a></body></html>\n\r";
    const char* mes = message.c_str();
    write(sock, mes, message.length());
    close(sock);
    close(listener);

	return 0;
}
