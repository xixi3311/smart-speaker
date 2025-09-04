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

    // ����д�˴򿪣���ֹMPlayer����������
    // g_fifo_fd = open("cmd_fifo", O_WRONLY | O_NONBLOCK);
    // if (g_fifo_fd == -1)
    // {
    //     perror("open cmd_fifo write end");
    //     exit(1);
    // }

    // ��ʼ��select��ѯ
    select_init();

    // ��ʼ���豸
    if (-1 == driver_init())
    {
        printf("driver init erro\n");
        // exit(1);
    }

    // ��ʼ�������ڴ�
    if (-1 == shm_init())
    {
        printf("share memery erro\n");
        // exit(1);
    }

    // ��������
    if (-1 == link_init())
    {
        printf("link init erro\n");
        return -1;
    }

    // ��ʼ������ TCP
    if (socket_init() == -1)
    {
        printf("socket connect erro\n");
        return -1;
    }

    // ��ȡ����
    get_music("random");

    // ��������
    // traverse_link();

    // printf("2\n");

    // printf("g_buttonfd=%d, g_serialfd=%d, g_socketfd=%d, g_maxfd=%d\n",
    //        g_buttonfd, g_serialfd, g_socketfd, g_maxfd);

    /* ���������õ� socket fd ���� select ���� */
    // FD_ZERO(&READSET);
    // FD_SET(0, &READSET);          // ��׼����
    // FD_SET(g_socketfd, &READSET); // socket
    // g_maxfd = g_socketfd > 0 ? g_socketfd : 0;

    // printf(">>> before select: fd 3 valid=%d\n", fcntl(3, F_GETFD)); // ���� 0 ��ʾ��-1 ��ʾ�ѹر�

    // ��������̸������̷��͵��ź�
    signal(SIGUSR1, update_music);

    m_select();

    // ��ȡ����
    while (1)
        ;
}
