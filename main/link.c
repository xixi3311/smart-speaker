#include "link.h"

extern Node *head;
extern int g_start_flag;
extern int g_socketfd;
extern fd_set READSET;

/*
    初始化链表
*/
int link_init()
{
    head = (Node *)malloc(sizeof(Node) * 1);
    if (NULL == head)
    {
        return -1;
    }

    head->next = NULL;
    head->prior = NULL;

    return 0;
}

/*
    解析服务器发来的命令获取歌曲名字并插入链表
*/
void create_link(char *s)
{
    // 解析命令 用array结构体数组存放
    struct json_object *obj = json_tokener_parse(s);
    if (NULL == obj)
    {
        printf("不是一个json字符串格式\n");
        return;
    }

    struct json_object *array;
    array = (struct json_object *)json_object_object_get(obj, "music");
    if (!array || !json_object_is_type(array, json_type_array))
    {
        printf("music 字段不存在或不是数组\n");
        json_object_put(obj);
        return;
    }

    int i;
    for (i = 0; i < 5; i++) // 每次接收music_number首歌
    {
        struct json_object *music = json_object_array_get_idx(array, i);
        if (insert_link((const char *)json_object_get_string(music)) == -1) // 插入链表
        {
            printf("插入链表失败\n");
            return;
        }
        json_object_put(music);
    }

    json_object_put(obj);
    json_object_put(array); // 释放对象
}

/*
    插入音乐进链表
*/
int insert_link(const char *name)
{
    Node *p = head;
    while (p->next)
    {
        p = p->next; // 遍历到尾部
    }

    /*
        新创一个节点来插入链表
    */
    Node *new = (Node *)malloc(sizeof(Node));
    if (NULL == new)
    {
        printf("malloc fail\n");
        return -1;
    }

    strcpy(new->music_name, name); // 拷贝音乐名称
    new->next = NULL;
    new->prior = p; // 插入链表尾部 节点为 （prior）（数据）（next）格式
    p->next = new;  // 链接

    return 0;
}

void traverse_link()
{
    Node *p = head->next;

    while (p)
    {
        printf("%s\n", p->music_name);
        p = p->next;
    }
}

/*
函数描述：找到下一首歌
函数参数：当前播放歌曲 播放模式 存放下一首歌
函数返回值：歌曲存在 返回0 不存在 返回-1
*/
int find_next_music(char *cur, int mode, char *next)
{
    Node *p = head->next;

    printf("next music\n");

    while (p)
    {
        if (strstr(p->music_name, cur) != NULL)
        {
            printf("next music name\n");
            break;
        }
        p = p->next;
    }

    // if (p == NULL)
    // {
    //     printf("current music not found in link list\n");
    //     return -1;
    // }

    if (mode == SHM_CYCLE) // 循环模式
    {
        printf("cycle mode\n");
        strcpy(next, p->music_name);
        return 0;
    }
    else if (mode == SHM_SEQUENCY) // 顺序播放
    {
        if (p->next != NULL)
        {
            printf("sequence mode\n");
            strcpy(next, p->next->music_name);
            return 0;
        }
        else
        {

            return -1;
            // // 重新获取第一首歌
            // if (head->next != NULL)
            // {
            //     strcpy(next, head->next->music_name);
            // }
        }
    }
}

/*
    清空链表
*/
void clear_link()
{
    Node *p = head->next;

    while (p)
    {
        head->next = p->next;
        free(p);
        p = head->next;
    }
}

/*
链表中的歌曲播放完毕 调用函数更新链表 （SIGUSR1触发）
*/
void update_music()
{
    g_start_flag = 0; // 修改标志位

    printf("[DEBUG] update_music() called by SIGUSR1\n");

    // 回收子进程 （这里容易出现僵尸进程）
    SHM s;
    get_shm(&s);

    printf("没头脑\n");

    int status;
    printf("子进程pid: %d\n", s.child_id);
    waitpid(s.child_id, &status, 0); // 回收子进程

    printf("大香蕉\n");

    clear_link();

    // 关闭旧的 socket
    // close(g_socketfd);
    // FD_CLR(g_socketfd, &READSET);

    // // 重新连接服务器
    // if (socket_init() == -1)
    // {
    //     printf("重新连接服务器失败\n");
    // }

    char singer[128] = {0};
    get_singer(singer);

    clear_link();

    // 请求新的歌曲数据
    get_music(singer);

    printf("[DEBUG] g_socketfd = %d\n", g_socketfd);

    // 开始播放
    start_play();
}

/*
获取歌曲歌手名称
*/
void get_singer(char *s)
{
    if (head->next == NULL)
        return;

    // 其他/倒带
    char *begin = head->next->music_name;
    char *p = begin;
    while (*p != '/')
        p++;

    strncpy(s, begin, p - begin);
}

/*
找到上一首歌：当前歌 上一首歌
*/
void find_prior_music(char *cur, char *prior)
{
    if (NULL == cur || NULL == prior)
        return;

    Node *p = head->next;

    if (strstr(p->music_name, cur))
    {
        strcpy(prior, p->music_name);
        return;
    }

    p = p->next;
    while (p)
    {
        if (strstr(p->music_name, cur))
        {
            strcpy(prior, p->prior->music_name);
            return;
        }
        p = p->next;
    }

    printf("遍历链表失败\n");
}
