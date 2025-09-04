#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <json-c/json.h>

#define N 20
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // 互斥锁
int i = 0;                                         // 统计目前连接成功的客户端总的数量

// 定义结构体存放连接成功的客户端信息
struct clientmsg
{
    char ip[20];            // 存放ip地址
    unsigned short portnum; // 存放端口号
    int sock;               // 存放套接字
};

// 定义结构体数组存放所有连接成功的客户端信息
struct clientmsg array[N];

/*
    回复客户端 歌曲数据
*/
void reply_music(int fd)
{
    /*
        服务器端向客户端回复歌曲数据
    */
    struct json_object *obj = (struct json_object *)json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("reply_music"));
    struct json_object *music_array = (struct json_object *)json_object_new_array();
    json_object_array_add(music_array, json_object_new_string("其他/不哭.mp3"));
    json_object_array_add(music_array, json_object_new_string("其他/倒带.mp3"));
    json_object_array_add(music_array, json_object_new_string("其他/红色高跟鞋.mp3"));
    json_object_array_add(music_array, json_object_new_string("其他/那年夏天宁静的海.mp3"));
    json_object_array_add(music_array, json_object_new_string("其他/鲜花.mp3"));
    json_object_object_add(obj, "music", music_array);

    char buf[1024] = {0};
    const char *s = (const char *)json_object_to_json_string(obj);
    int len = strlen(s);
    memcpy(buf, &len, 4);
    memcpy(buf + 4, s, len); // 前4个字节保存的是长度信息 从buf + 4开始写入

    send(fd, buf, len + 4, 0);
}

// 线程的任务函数--》接收客户端发送过来的信息
void *recvtask(void *arg)
{
    struct clientmsg *cmsg = (struct clientmsg *)arg;
    char rbuf[1024];
    int idx = -1; // 用于记录当前客户端在数组中的索引

    // 查找当前客户端在数组中的索引
    for (int j = 0; j < i; j++)
    {
        if (array[j].sock == cmsg->sock)
        {
            idx = j;
            break;
        }
    }

    while (1)
    {
        bzero(rbuf, 1024);
        // 接收客户端发送过来的信息
        int len;
        int ret = recv(cmsg->sock, &len, 4, 0);
        if (ret == -1)
        {
            perror("recv erro");
        }
        else if (ret == 0)
        {
            printf("异常退出\n");
            break;
        }
        ret = recv(cmsg->sock, rbuf, len, 0);
        if (ret == 0 || ret == -1)
        {
            printf("客户端%s 端口%hu断开连接!\n", cmsg->ip, cmsg->portnum);
            pthread_mutex_lock(&mutex); // 多线程互斥锁
            // 从数组中删除该客户端信息
            for (int j = idx; j < i - 1; j++)
            {
                array[j] = array[j + 1];
            }
            i--;
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL); // 客户端断开，线程退出
        }
        printf("客户端%s 端口%hu发送过来的信息是:%s\n", cmsg->ip, cmsg->portnum, rbuf);

        struct json_object *json = (struct json_object *)json_tokener_parse(rbuf);
        struct json_object *val = (struct json_object *)json_object_object_get(json, "cmd");
        const char *s = (const char *)json_object_get_string(val);

        if (!strcmp(s, "get_music_list")) // 比较是不是这个命令来给予回复
        {
            reply_music(cmsg->sock);
        }
    }
}

int main()
{
    pthread_t id, id1;
    int tcpsock;
    int ret;

    // 定义ipv4地址结构体变量存放要绑定的ip和端口号
    struct sockaddr_in bindaddr;
    bzero(&bindaddr, sizeof(bindaddr));
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = inet_addr("192.168.232.134"); // 绑定服务器自己的ip地址
    bindaddr.sin_port = htons(10010);                        // 服务器的端口号

    // 定义ipv4地址结构体变量存放连接成功的客户端ip和端口号
    struct sockaddr_in clientaddr;
    bzero(&clientaddr, sizeof(clientaddr));
    int size = sizeof(clientaddr);

    // 创建tcp套接字
    tcpsock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcpsock == -1)
    {
        printf("创建tcp套接字失败了!\n");
        return -1;
    }

    // 绑定自己的ip和端口号
    ret = bind(tcpsock, (struct sockaddr *)&bindaddr, sizeof(bindaddr));
    if (ret == -1)
    {
        printf("绑定ip或者端口号失败了!\n");
        return -1;
    }

    // 监听
    ret = listen(tcpsock, 10);
    if (ret == -1)
    {
        printf("监听失败了!\n");
        return -1;
    }

    printf("服务器启动成功，等待客户端连接...\n");

    while (1)
    {
        // 循环接收客户端的连接请求--》愿意接听
        int newsock = accept(tcpsock, (struct sockaddr *)&clientaddr, &size);
        if (newsock == -1)
        {
            printf("接收客户端连接请求失败了!\n");
            return -1;
        }

        // 打印目前连接成功的客户端ip，端口号，新的套接字
        printf("目前连接成功的客户端ip地址是: %s\n", inet_ntoa(clientaddr.sin_addr));
        printf("目前连接成功的客户端端口号是: %hu\n", ntohs(clientaddr.sin_port));
        printf("目前连接成功的客户端新的套接字是: %d\n", newsock);

        // 把连接成功的客户端信息存放到结构体数组中
        strcpy(array[i].ip, inet_ntoa(clientaddr.sin_addr)); // 存放ip地址
        array[i].portnum = ntohs(clientaddr.sin_port);       // 存放端口号
        array[i].sock = newsock;                             // 存放套接字

        // 创建一个线程--》专门接收这个客户端（连接成功的客户端）发送过来的信息
        pthread_create(&id1, NULL, recvtask, &array[i]);
        i++;

        sleep(5);

        const char *s = "{'cmd' : 'app_start'}";
        char buf[1024] = {0};
        int len = strlen(s);
        memcpy(buf, &len, 4);
        memcpy(buf + 4, s, len);

        send(newsock, buf, len + 4, 0);

        sleep(3);

        s = "{'cmd' : 'app_next'}";
        memset(buf, 0, 1024);
        len = strlen(s);
        memcpy(buf, &len, 4);
        memcpy(buf + 4, s, len);

        send(newsock, buf, len + 4, 0);

        sleep(3);

        s = "{'cmd' : 'app_voice_up'}";
        memset(buf, 0, 1024);
        len = strlen(s);
        memcpy(buf, &len, 4);
        memcpy(buf + 4, s, len);

        send(newsock, buf, len + 4, 0);

        sleep(3);

        s = "{'cmd' : 'app_circle'}";
        memset(buf, 0, 1024);
        len = strlen(s);
        memcpy(buf, &len, 4);
        memcpy(buf + 4, s, len);

        send(newsock, buf, len + 4, 0);

        sleep(3);

        s = "{'cmd' : 'app_prior'}";
        memset(buf, 0, 1024);
        len = strlen(s);
        memcpy(buf, &len, 4);
        memcpy(buf + 4, s, len);

        send(newsock, buf, len + 4, 0);
    }

    // 关闭套接字
    close(tcpsock);
    return 0;
}