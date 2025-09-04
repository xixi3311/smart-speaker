#include "socket.h"

extern fd_set READSET;
extern int g_maxfd;
extern Node *head;
extern int g_start_flag;
extern int g_suspend_flag;

int g_socketfd = 0;

/*
    音箱向服务器发出信息
    音量 正在播放的歌曲 设备号 模式
    函数返回值：没有
    参数：信号值
*/
void send_server(int sig)
{
    struct json_object *SendObject = json_object_new_object();
    json_object_object_add(SendObject, "cmd", json_object_new_string("info"));

    // 获取当前播放的音乐
    SHM s;
    memset(&s, 0x00, sizeof(s));
    get_shm(&s);
    json_object_object_add(SendObject, "cur_music", json_object_new_string(s.cur_music));
    json_object_object_add(SendObject, "mode", json_object_new_int(s.mod));

    // 获取当前系统音量
    json_object_object_add(SendObject, "volume", json_object_new_int(get_volume_percent()));

    // 播放状态
    if (g_start_flag == 0)
    {
        json_object_object_add(SendObject, "status", json_object_new_string("stop"));
    }
    else if (g_start_flag == 1 && g_suspend_flag == 1)
    {
        json_object_object_add(SendObject, "status", json_object_new_string("suspend"));
    }
    else if (g_start_flag == 1 && g_suspend_flag == 0)
    {
        json_object_object_add(SendObject, "status", json_object_new_string("start"));
    }

    // 获取设备id 多台设备
    json_object_object_add(SendObject, "deviceid", json_object_new_string("0001"));

    // 发送数据到服务器
    socket_send_data(SendObject);

    // 用完后释放对象
    json_object_put(SendObject);

    alarm(2); // 每隔2秒上报数据
}

/*
    发送数据  ： json长度 + json对象
    避免发送不完整 先发长度 不然只接受一部分
*/
void socket_send_data(json_object *j)
{
    int len;
    char msg[1024] = {0};
    char *s = json_object_to_json_string(j);
    len = strlen(s);

    memcpy(msg, &len, sizeof(int)); // 先发送字符串长度
    memcpy(msg + sizeof(int), s, len);

    if (send(g_socketfd, msg, len + 4, 0) == -1)
    {
        perror("send erro");
    }
}

/*
    每次更新完链表 都要通知APP
*/
void upload_music_list()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("upload_music"));

    struct json_object *array = json_object_new_array();
    Node *p = head->next;

    while (p)
    {
        json_object_array_add(array, json_object_new_string(p->music_name));
        p = p->next;
    }

    json_object_object_add(obj, "music", array);

    socket_send_data(obj);

    json_object_put(obj);
    json_object_put(array);
}

/*
    接收服务器数据
*/
void socket_recv_data(char *msg)
{
    // 获取前面4字节长度信息
    int len;
    int size = 0;
    char buf[1024] = {0};

    while (1)
    {
        // 每次从buf + size开始接收，初始化size = 0， 如果sizeof（int）- size恰好为
        // 4字节也就是sizeof（int） 那就完美 不然size + 偏移量
        size += recv(g_socketfd, buf + size, sizeof(int) - size, 0);
        if (size >= sizeof(int))
            break;
    }

    len = *(int *)buf;
    printf("-----LENGTH : %d\n", len);

    // 根据长度信息接收len长度的json数据 避免只接收到一段
    size = 0;
    while (1)
    {
        size += recv(g_socketfd, msg + size, len - size, 0);
        if (size >= len)
            break;
    }

    printf("------MSG : %s\n", msg);
}

/*
    网络初始化
*/
int socket_init()
{
    int ret;
    int count = 50;

    printf("READSET addr = %p\n", (void *)&READSET);

    g_socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (-1 == g_socketfd)
    {
        perror("socket erro");
        return -1;
    }

    struct sockaddr_in serveraddr;
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = inet_addr(SERVERADDR); // 大小端转换

    int len = sizeof(serveraddr);

    // 向服务器发起连接信号
    while (count--)
    {
        if (-1 == connect(g_socketfd, (struct sockaddr *)&serveraddr, len))
        {
            printf("connect fail...\n");
            sleep(1);
            continue;
        }

#ifdef ARM
        // 蜂鸣器0.5秒提示
        start_buzzer();
#endif

        printf("connect successfully\n");
        g_maxfd = (g_maxfd < g_socketfd) ? g_socketfd : g_maxfd;
        FD_SET(g_socketfd, &READSET);

        // 音箱每隔2秒上报一次数据
        // alarm(2);
        signal(SIGALRM, send_server);
        send_server(SIGALRM);

        return 0;
    }

    return -1;
}

/*
    app请求音箱播放数据 并上报app播放成功
*/
int socket_start_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_start_reply"));

    if (start_play() == -1)
    {
        json_object_object_add(obj, "result", json_object_new_string("failure"));
    }
    else
    {
        json_object_object_add(obj, "result", json_object_new_string("success"));
    }

    socket_send_data(obj);

    json_object_put(obj);
}

/*
    服务器请求 上报APP停止播放
*/
void socket_stop_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_stop_reply"));

    stop_play();

    json_object_object_add(obj, "result", json_object_new_string("success"));

    socket_send_data(obj);

    json_object_put(obj);
}

/*
    服务器请求 上报APP暂停播放
*/
void socket_suspend_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_suspend_reply"));

    suspend_play();

    json_object_object_add(obj, "result", json_object_new_string("success"));

    socket_send_data(obj);

    json_object_put(obj);
}

/*
    服务器请求 上报APP继续播放
*/
void socket_continue_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_continue_reply"));

    continue_play();

    json_object_object_add(obj, "result", json_object_new_string("success"));

    socket_send_data(obj);

    json_object_put(obj);
}

/*
    服务器请求 上报APP上一首歌播放
*/
void socket_prior_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_prior_reply"));

    prior_play();

    json_object_object_add(obj, "result", json_object_new_string("success"));

    SHM s;
    get_shm(&s);
    json_object_object_add(obj, "music", json_object_new_string(s.cur_music));

    socket_send_data(obj);

    json_object_put(obj);
}

/*
    服务器请求 上报APP下一首歌播放
*/
void socket_next_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_next_reply"));

    next_play();

    json_object_object_add(obj, "result", json_object_new_string("success"));

    SHM s;
    get_shm(&s);
    json_object_object_add(obj, "music", json_object_new_string(s.cur_music));

    socket_send_data(obj);

    json_object_put(obj);
}

/*
    服务器请求 上报APP音量增加5%
*/
void socket_voice_up()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_voice_up_reply"));

    set_volume_percent(get_volume_percent() + 5);

    json_object_object_add(obj, "result", json_object_new_string("success"));

    int volume;
    volume = get_volume_percent();
    json_object_object_add(obj, "voice", json_object_new_int(volume));

    socket_send_data(obj);

    json_object_put(obj);
}

/*
    服务器请求  上报APP音量减小
*/
void socket_voice_dowm()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_voice_down_reply"));

    set_volume_percent(get_volume_percent() - 5);

    json_object_object_add(obj, "result", json_object_new_string("success"));

    int volume;
    volume = get_volume_percent();
    json_object_object_add(obj, "voice", json_object_new_int(volume));

    socket_send_data(obj);

    json_object_put(obj);
}

void socket_circle_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_circle_reply"));

    circle_play();

    json_object_object_add(obj, "result", json_object_new_string("success"));

    socket_send_data(obj);

    json_object_put(obj);
}

void socket_sequence_play()
{
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("app_sequence_reply"));

    sequence_play();

    json_object_object_add(obj, "result", json_object_new_string("success"));

    socket_send_data(obj);

    json_object_put(obj);
}
