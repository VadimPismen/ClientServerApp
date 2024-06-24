// LinuxClient.cpp: определяет точку входа для приложения.
//

#include "LinuxClient.h"

using namespace std;

int main()
{
    int sock;
    struct sockaddr_in addr;
    int bytes_read;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(3425);
    addr.sin_addr.s_addr = inet_addr("192.168.4.201");
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        perror("connect");
        exit(2);
    }

    char buf[1024];
    bytes_read = recv(sock, buf, 1024, 0);
    cout << buf;
    close(sock);

	return 0;
}
