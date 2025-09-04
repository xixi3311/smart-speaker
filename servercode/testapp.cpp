#include <stdio.h>
#include <jsoncpp/json/json.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <pthread.h>
using namespace std;

int sockfd;

#define IP "172.25.82.12"
#define PORT 10001 // Ҫ�����������

void *receive(void *arg)
{
    char buf[1024] = {0};
    int len;

    while (1)
    {
        recv(sockfd, &len, 4, 0);
        recv(sockfd, buf, len, 4);
        printf("%s\n", buf);
    }
}

void send_server(int sig)
{
    Json::Value v;
    v["cmd"] = "app_info";
    v["appid"] = "1001";
    v["deviceid"] = "0001";

    char buf[1024] = {0};
    string str = Json::FastWriter().write(v);
    int len = str.size();
    memcpy(buf, &len, 4);
    memcpy(buf + 4, str.c_str(), len);

    send(sockfd, buf, len + 4, 0);

    alarm(2);
}

int main()
{
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_info;
    int len = sizeof(server_info);
    memset(&server_info, 0, len);
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(PORT);
    server_info.sin_addr.s_addr = inet_addr(IP);

    if (connect(sockfd, (struct sockaddr *)&server_info, len) == -1)
    {
        perror("connect");
        return -1;
    }

    signal(SIGALRM, send_server);
    send_server(SIGALRM);

    pthread_t tid;
    pthread_create(&tid, NULL, receive, NULL);

    getchar();

    Json::Value v;
    v["cmd"] = "app_offline";
    v["appid"] = "3333";
    v["deviceid"] = "0001";

    char buf[1024] = {0};
    string str = Json::FastWriter().write(v);
    len = str.size();
    memcpy(buf, &len, 4);
    memcpy(buf + 4, str.c_str(), len);

    send(sockfd, buf, len + 4, 0);

    while (1)
        ;

    return 0;
}