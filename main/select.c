#include "select.h"

extern fd_set READSET;
extern int g_maxfd;
extern int g_buttonfd;
extern int g_serialfd;
extern int g_socketfd;

void select_init()
{
    FD_ZERO(&READSET);

    FD_SET(0, &READSET); // 把fd为0输入 加入select集合中

    // printf("select_init: set = ");
    // for (int fd = 0; fd <= g_maxfd; ++fd)
    //     if (FD_ISSET(fd, &READSET))
    //         printf("%d ", fd);
    // printf("\n");
}

/*
    读取标准输入
*/
void select_read_stdio()
{
    char ch;
    // scanf("%c", &ch);
    // read(0, &ch, 1); // 非阻塞读取一个字符
    printf("xixi\n");
    scanf("%c", &ch);

    // 清空缓冲区（处理换行符等残留字符）
    while (getchar() != '\n')
        ;

    switch (ch)
    {
    case '1':
        start_play();
        break;
    case '2':
        stop_play();
        break;
    case '3':
        suspend_play();
        break;
    case '4':
        continue_play();
        break;
    case '5':
        next_play();
        break;
    case '6':
        prior_play();
        break;
    case '7':
        set_volume_percent(get_volume_percent() + 5);
        printf("Current volume: %d%%\n", get_volume_percent());
        break;
    case '8':
        set_volume_percent(get_volume_percent() - 5);
        printf("Current volume: %d%%\n", get_volume_percent());
        break;
    case '9':
        circle_play();
        break;
    case 'a':
        sequence_play();
        break;
    case 'b':
        singer_play("其他");
        break;
    }
}

/*
    显示菜单
*/
void show_menu()
{
    printf("***************************************\n");
    printf("     1.开始播放         2.结束播放\n");
    printf("     3.暂停播放         4.继续播放\n");
    printf("     5.下一首           6.上一首\n");
    printf("     7.增加音量         8.减小音量\n");
    printf("     9.单曲循环         a.顺序播放\n");
    printf("***************************************\n");
}

/*
按键监听
*/
void select_read_button()
{
    int key = get_key_id();
    printf("读到按键。。。\n");

    switch (key)
    {
    case 1:
        start_play();
        break;
    case 2:
        stop_play();
        break;
    case 3:
        suspend_play();
        break;
    case 4:
        continue_play();
        break;
    case 5:
        next_play();
        break;
    case 6:
        prior_play();
        break;
    }
}

void select_read_serial()
{
    printf("--  串口到数据  --\n");
    char ch;

    if (read(g_serialfd, &ch, 1) == -1)
    {
        perror("read");
        return;
    }

    switch (ch)
    {
    case 1:
        start_play();
        break;
    case 2:
        stop_play();
        break;
    case 3:
        suspend_play();
        break;
    case 4:
        continue_play();
        break;
    case 5:
        prior_play();
        break;
    case 6:
        next_play();
        break;
    case 7:
        set_volume_percent(get_volume_percent() + 5);
        printf("Current volume: %d%%\n", get_volume_percent());
        break;
    case 8:
        set_volume_percent(get_volume_percent() - 5);
        printf("Current volume: %d%%\n", get_volume_percent());
        break;
    case 9:
        circle_play();
        break;
    case 0x0a:
        sequence_play();
        break;
    case 0x0b:
        singer_play("王心凌");
        break;
    }
}

/*
    接收cmd命令
*/
int parse_message(char *msg, char *cmd)
{
    struct json_object *obj = (struct json_object *)json_tokener_parse(msg);
    if (NULL == obj)
    {
        printf("json_tokener_parse\n");
        return -1;
    }

    struct json_object *value;
    value = (struct json_object *)json_object_object_get(obj, "cmd");
    if (NULL == value)
    {
        printf("不是cmd字段\n");
        return -1;
    }

    strcpy(cmd, (const char *)json_object_get_string(value));
}

/*
    select接收服务器数据
*/
void socket_select_recv_data()
{
    char buf[1024] = {0};
    char cmd[128] = {0};

    socket_recv_data(buf); // json格式字符串 {"cmd": "start"}

    if (parse_message(buf, cmd) == -1)
    {
        printf("收到的不是josn数据格式\n");
    }
    else
    {
        printf("cmd %s\n", cmd);
    }

    if (!strcmp(cmd, "app_start")) // 开始播放
    {
        socket_start_play();
    }
    else if (!strcmp(cmd, "app_stop")) // 停止播放
    {
        socket_stop_play();
    }
    else if (!strcmp(cmd, "app_suspend")) // 暂停播放
    {
        socket_suspend_play();
    }
    else if (!strcmp(cmd, "app_continue")) // 继续播放
    {
        socket_continue_play();
    }
    else if (!strcmp(cmd, "app_prior")) // 上一首歌
    {
        socket_prior_play();
    }
    else if (!strcmp(cmd, "app_next")) // 下一首歌
    {
        socket_next_play();
    }
    else if (!strcmp(cmd, "app_voice_up")) // 增加音量
    {
        socket_voice_up();
    }
    else if (!strcmp(cmd, "app_voice_down")) // 减小音量
    {
        socket_voice_dowm();
    }
    else if (!strcmp(cmd, "app_circle")) // 循环播放
    {
        socket_circle_play();
    }
    else if (!strcmp(cmd, "app_sequence")) // 顺序播放
    {
        socket_sequence_play();
    }
    else if (!strcmp(cmd, "app_get_music"))
    {
        upload_music_list();
    }
}

/*
    select监听
*/
void m_select()
{

    show_menu();

    // printf("READSET addr = %p\n", (void *)&READSET);

    // printf(">>> m_select set = ");
    // for (int fd = 0; fd <= g_maxfd; ++fd)
    //     if (FD_ISSET(fd, &READSET))
    //         printf("%d ", fd);
    // printf("\n");

    fd_set tempset;

    // printf("g_maxfd: %d", g_maxfd);

    // printf(">>> before select: fd 3 valid=%d\n",
    //        fcntl(3, F_GETFD)); // 返回 0 表示存活，-1 表示已关闭

    while (1)
    {
        tempset = READSET; // 使用临时变量存储

        printf("等待输入。。。\n");

        int ret = select(g_maxfd + 1, &tempset, NULL, NULL, NULL);
        if (ret == -1 && errno == EINTR) // errno == EINTR是信号来而不是错误
        {                                // 代码中有alarm信号
            continue;
        }
        else if (ret == -1 && errno != EINTR)
        {
            // printf("g_maxfd: %d", g_maxfd);
            // printf("select errno=%d (%s)\n", errno, strerror(errno));
            perror("select erro");
            return;
        }

        if (FD_ISSET(0, &tempset)) // 键盘
        {
            select_read_stdio();
        }
        // else if (FD_ISSET(g_buttonfd, &tempset)) // 按键
        // {
        //        select_read_button();
        // }
        // else if (FD_ISSET(g_serialfd, &tempset)) // 语音
        // {
        //    select_read_serial();
        // }
        else if (FD_ISSET(g_socketfd, &tempset)) // socket
        {
            socket_select_recv_data();
        }
    }
}