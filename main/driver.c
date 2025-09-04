#include "driver.h"

extern int g_maxfd;
extern fd_set READSET;

int g_buttonfd = -1;
int g_pcmfd = -1;
int g_serialfd = -1;
int g_buzzefd = -1;

int driver_init()
{
    // ��ʼ����Ƶ�豸
    if (open_master() < 0)
    {
        fprintf(stderr, "Cannot open 'Master' mixer element!\n");
        return 1;
    }

    // g_maxfd = (g_maxfd < g_pcmfd) ? g_pcmfd : g_maxfd;

#ifdef ARM
    ��ʼ������
    g_buttonfd = open("/dev/buttons", O_RDONLY);
    if (-1 == g_buttonfd)
    {
        perror("open buttons erro");
        return -1;
    }

    ������󼯺�
    g_maxfd = (g_maxfd < g_buttonfd) ? g_buttonfd : g_maxfd;
    �Ѱ���������ѯ�������ڼ���
    FD_SET(g_buttonfd, &READSET);

    ��ʼ������
    g_serialfd = open("/dev/ttyS0", O_RDONLY);
    if (-1 == g_serialfd)
    {
        perror("open serial erro");
        return -1;
    }

    if (init_serial() == -1)
    {
        printf("���ڳ�ʼ��ʧ��\n");
        return -1;
    }

    ������󼯺�
    g_maxfd = (g_maxfd < g_serialfd) ? g_serialfd : g_maxfd;
    FD_SET(g_serialfd, &READSET);

    ��ʼ��������
    g_buzzefd = open("/dev/pwm", O_WRONLY);
    if (-1 == g_buzzefd)
    {
        perror("open pwm erro");
        return -1;
    }

    ������󼯺�
    g_maxfd = (g_maxfd < g_buzzefd) ? g_buzzefd : g_maxfd;

#endif

    return 0;
}

/*
������
*/
void start_buzzer()
{
    int freq = 1000;
    int ret = ioctl(g_buzzefd, 1, &freq);
    if (-1 == ret)
    {
        perror("ioctl");
        return;
    }

    usleep(500000);

    ret = ioctl(g_buzzefd, 0);
    if (-1 == ret)
    {
        perror("ioctl");
    }
}

/*
���� 6������
*/
int get_key_id()
{
    char buttons[6] = {'0', '0', '0', '0', '0', '0'};
    char cur_buttons[6] = {0};

    ssize_t size = read(g_buttonfd, cur_buttons, 6);
    if (-1 == size)
    {
        perror("read");
        return -1;
    }

    int i;
    for (i = 0; i < 6; i++)
    {
        if (buttons[i] != cur_buttons[i])
        {
            return i + 1;
        }
    }

    return -1;
}

/*
    ��ʼ������
*/
int init_serial()
{
    struct termios TtyAttr;

    memset(&TtyAttr, 0, sizeof(struct termios));
    TtyAttr.c_iflag = IGNPAR;
    TtyAttr.c_cflag = B115200 | HUPCL | CS8 | CREAD | CLOCAL;
    TtyAttr.c_cc[VMIN] = 1; // ֹͣλ

    if (tcsetattr(g_serialfd, TCSANOW, &TtyAttr) == -1)
    {
        perror("tcsetattr");
        return -1;
    }
}
