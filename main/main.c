#include "main.h"

fd_set READSET;
int g_maxfd = 0;
Node *head = NULL;

extern int g_buttonfd;
extern int g_serialfd;
extern int g_socketfd;

// int g_fifo_fd = -1;

int main()
{
    system("./init.sh");

    // int fifo_fd = open("cmd_fifo", O_WRONLY | O_NONBLOCK);
    // if (fifo_fd == -1)
    // {
    //     perror("open fifo error");
    // }

    // 保持写端打开（防止MPlayer读端阻塞）
    // g_fifo_fd = open("cmd_fifo", O_WRONLY | O_NONBLOCK);
    // if (g_fifo_fd == -1)
    // {
    //     perror("open cmd_fifo write end");
    //     exit(1);
    // }

    // 初始化select轮询
    select_init();

    // 初始化设备
    if (-1 == driver_init())
    {
        printf("driver init erro\n");
        // exit(1);
    }

    // 初始化共享内存
    if (-1 == shm_init())
    {
        printf("share memery erro\n");
        // exit(1);
    }

    // 建立链表
    if (-1 == link_init())
    {
        printf("link init erro\n");
        return -1;
    }

    // 初始化网络 TCP
    if (socket_init() == -1)
    {
        printf("socket connect erro\n");
        return -1;
    }

    // 获取音乐
    get_music("random");

    // 遍历链表
    // traverse_link();

    // printf("2\n");

    // printf("g_buttonfd=%d, g_serialfd=%d, g_socketfd=%d, g_maxfd=%d\n",
    //        g_buttonfd, g_serialfd, g_socketfd, g_maxfd);

    /* 把真正可用的 socket fd 加入 select 集合 */
    // FD_ZERO(&READSET);
    // FD_SET(0, &READSET);          // 标准输入
    // FD_SET(g_socketfd, &READSET); // socket
    // g_maxfd = g_socketfd > 0 ? g_socketfd : 0;

    // printf(">>> before select: fd 3 valid=%d\n", fcntl(3, F_GETFD)); // 返回 0 表示存活，-1 表示已关闭

    // 处理孙进程给父进程发送的信号
    signal(SIGUSR1, update_music);

    m_select();

    // 获取音乐
    while (1)
        ;
}
