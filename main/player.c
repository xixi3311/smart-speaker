#include "player.h"

int g_shmid = 0;        // 共享内存句柄
int g_start_flag = 0;   // 开始播放标志位
int g_suspend_flag = 0; // 暂停播放标志位

extern int g_maxfd;
extern Node *head;

/*
    初始化共享内存
*/
int shm_init()
{
    // 创建共享内存
    g_shmid = shmget(SHMKEY, SHMSIZE, IPC_CREAT | IPC_EXCL);
    if (-1 == g_shmid)
    {
        perror("shmget erro");
        return -1;
    }

    // 映射
    void *addr = shmat(g_shmid, NULL, 0);
    if ((void *)-1 == addr)
    {
        perror("shmat erro");
        return -1;
    }

    SHM s;
    memset(&s, 0, sizeof(s));
    s.parent_id = getpid();
    s.mod = SHM_SEQUENCY;
    memcpy(addr, &s, sizeof(s)); // 把数据写入共享内存

    // 解除映射
    shmdt(addr);
}

// 读取共享内存中的当前状态（如当前播放的歌曲、播放模式等），并保存到 s 中供使用。
void get_shm(SHM *s)
{
    void *addr = shmat(g_shmid, NULL, 0);
    if ((void *)-1 == addr)
    {
        perror("shmat erro");
        return;
    }

    memcpy(s, addr, sizeof(SHM));

    shmdt(addr);
}

/*
    设置共享内存
*/
void set_shm(SHM s)
{
    void *addr = shmat(g_shmid, NULL, 0);
    if ((void *)-1 == addr)
    {
        perror("shmat erro");
        return;
    }

    memcpy(addr, &s, sizeof(SHM));

    shmdt(addr);
}

/*
    获取音量大小
*/
void get_volum(int *volum)
{
    long vol = 0;
    snd_mixer_selem_get_playback_volume(playback_vol_element, SND_MIXER_SCHN_FRONT_LEFT, &vol);
    *volum = (int)vol;
}

/*
    向服务器发送获取音乐信息
*/
void get_music(const char *singer)
{
    // 发送请求
    struct json_object *obj = json_object_new_object();
    json_object_object_add(obj, "cmd", json_object_new_string("get_music_list"));
    json_object_object_add(obj, "singer", json_object_new_string(singer));

    socket_send_data(obj);

    char msg[1024] = {0};
    socket_recv_data(msg);

    // 形成链表
    create_link(msg);

    // 上传音乐数据到服务器端
    upload_music_list();
}

/*
开始播放歌曲
*/
int start_play()
{
    if (g_start_flag == 1) // 已经开始播放
    {
        return -1;
    }

    if (head->next == NULL)
    {
        return -1;
    }

    char name[32] = {0};
    strcpy(name, head->next->music_name);

    // 初始化音量
    //  int volum = DEF_VOL;
    //  volum *= 257;
    //  ioctl(g_mixerfd, SOUND_MIXER_WRITE_VOLUME, &volum);

    g_start_flag = 1;

    play_music(name);

    return 0;
}

/*子进程
1、创建孙进程，调用mplyer播放音乐
2、等待孙进程结束
*/
void child_process(char *n)
{
    printf("g_start_flag是: %d\n", g_start_flag);
    while (g_start_flag)
    {
        pid_t grand_pid = fork(); // 子进程创建孙进程
        if (grand_pid == -1)
        {
            perror("grand fork");
            return;
        }
        else if (grand_pid == 0) // 孙进程
        {
            // 关闭标准输入
            close(0);
            // 重定向标准输入到 /dev/null
            open("/dev/null", O_RDONLY);

            SHM s;
            memset(&s, 0, sizeof(s));

            if (strlen(n) == 0) // 第二次循环 （自动播放下一首）
            {
                grand_get_shm(&s);

                if (find_next_music(s.cur_music, s.mod, n) == -1)
                {
                    // 歌曲播放完了 通知父子进程
                    kill(s.parent_id, SIGUSR1); // 用SIGUSR1信号通知父进程

                    printf("通知SIGUSR2\n");
                    kill(s.child_id, SIGUSR2); // 用SIGUSR2信号通知子进程
                    // 不能用SIGUSR1 因为父进程用了 子进程复制

                    printf("通知SIGUSR2完了!!\n");

                    usleep(100000); // 进程间切换 睡眠一下 避免太快exit退出

                    exit(0); // 孙进程退出
                }
            }

            char *arg[7] = {0};
            char music_path[128] = {0};
            strcpy(music_path, URL);
            strcat(music_path, n);

            arg[0] = "mplayer";
            arg[1] = music_path;
            arg[2] = "-slave";
            arg[3] = "-quiet";
            arg[4] = "-input";
            arg[5] = "file=./cmd_fifo";
            arg[6] = NULL;

            // 修改共享内存
            grand_get_shm(&s);
            char *p = n;
            while (*p != '/')
                p++;
            strcpy(s.cur_music, p + 1);
            grand_set_shm(s);

            execv("/usr/bin/mplayer", arg);
        }
        else
        {
            memset(n, 0, sizeof(n)); // 等待孙进程播放完清空n 和上面对应

            int status;
            wait(&status); // 等待回收孙进程

            usleep(100000);
        }
    }
    printf("g_start_flag是: %d\n", g_start_flag);
}

void grand_set_shm(SHM s)
{
    // 修改共享内存  (子进程id 孙进程id 音乐名称)
    int shmid = shmget(SHMKEY, SHMSIZE, 0);
    if (-1 == shmid)
    {
        perror("grand shmget");
        return;
    }

    void *addr = shmat(shmid, NULL, 0);
    if ((void *)-1 == addr)
    {
        perror("grand shmgat");
        return;
    }

    s.child_id = getppid();
    s.grand_id = getpid();

    memcpy(addr, &s, sizeof(s));

    printf("shared memory updated: child_id=%d, grand_id=%d, cur_music=%s, mod=%d\n",
           s.child_id, s.grand_id, s.cur_music, s.mod);

    shmdt(addr);
}

void grand_get_shm(SHM *s)
{
    int shmid = shmget(SHMKEY, SHMSIZE, 0);
    if (-1 == shmid)
    {
        perror("grand shmget");
        return;
    }

    void *addr = shmat(shmid, NULL, 0);
    if ((void *)-1 == addr)
    {
        perror("grand shmgat");
        return;
    }

    memcpy(s, addr, sizeof(SHM));

    printf("第二次循环3\n");

    shmdt(addr);
}

/*
孙进程通知子进程
*/
void child_quit()
{
    g_start_flag = 0; // 把子进程的g_start_flag开始播放设置为0 进不了循环
    printf("g_start_flag 运行了吗\n");
}

/*
    播放音乐
*/
void play_music(char *n)
{
    pid_t child_pid = fork(); // 父进程创建子进程

    if (-1 == child_pid)
    {
        perror("child fork erro");
        return;
    }
    else if (0 == child_pid) // 说明为子进程 子进程执行这个
    {
        close(0);                    // 关闭标准输入(键盘) 否则父进程就无法输入了
        open("/dev/null", O_RDONLY); // 防止后续误用

        signal(SIGUSR2, child_quit);

        printf("结束了\n");

        child_process(n);

        printf("child_process运行结束了\n");
        exit(0); // 这里一定要退出 不然会update_music函数会一直wait
    }
    else
    {
        return; // 这里还不能直接回收子进程
    }
}

/*
写入管道命令
函数参数：命令
*/
void write_fifo(const char *cmd)
{
    int fd = open("cmd_fifo", O_WRONLY);
    if (-1 == fd)
    {
        perror("fifo open");
        return;
    }

    if (write(fd, cmd, strlen(cmd)) == -1)
    {
        perror("fifo write");
        return;
    }

    close(fd);
}

/*
结束播放
*/
void stop_play()
{
    if (g_start_flag == 0)
        return;

    // 通知子进程
    SHM s;
    get_shm(&s);
    kill(s.child_id, SIGUSR2);

    // 结束mplyer （管道stop）
    write_fifo("stop\n");

    // 回收子进程
    int status;
    waitpid(s.child_id, &status, 0);

    // 设置标志位
    g_start_flag = 0;
}

void suspend_play()
{
    if (g_start_flag == 0 || g_suspend_flag == 1)
        return;

    printf("--  暂停播放 --\n");

    write_fifo("pause\n");

    g_suspend_flag = 1;
}

void continue_play()
{
    if (g_start_flag == 0 || g_suspend_flag == 0)
        return;

    printf("--  开始播放 --\n");

    write_fifo("pause\n");

    g_suspend_flag = 0;
}

void next_play()
{
    // if (g_start_flag = 0)
    //     return;

    // SHM s;
    // get_shm(&s);
    // char music[128] = {0};
    // if (find_next_music(s.cur_music, SHM_SEQUENCY, music) == -1)
    // {
    //     // 链表里面歌曲播放完了
    //     stop_play();

    //     char singer[128] = {0};
    //     get_singer(singer);

    //     clear_link();
    //     get_music(singer);
    //     start_play();

    //     if (g_suspend_flag == 1)
    //         g_suspend_flag = 0;

    //     return;
    // }

    // char path[256] = {0};
    // strcat(path, URL);
    // strcat(path, music);

    // char cmd[256] = {0};
    // sprintf(cmd, "loadfile %s\n", path);

    // write_fifo(cmd);

    // // 更新共享内存 不然一直卡在第二首切第三首歌
    // int i;
    // for (i = 0; i < sizeof(music); i++)
    // {
    //     if (music[i] == '/')
    //         break;
    // }
    // strcpy(s.cur_music, music + i + 1);
    // set_shm(s);

    // if (g_suspend_flag == 1)
    //     g_suspend_flag = 0;
    /* 1. 判断当前是否处于播放状态（注意 == 误写 = 的 bug） */
    if (g_start_flag == 0)
        return;

    SHM s;
    get_shm(&s);
    char music[128] = {0};

    /* 2. 找下一首 */
    if (find_next_music(s.cur_music, SHM_SEQUENCY, music) == -1)
    {
        /* 列表播放完毕：重新拉歌并从头播放 */
        stop_play();

        char singer[128] = {0};
        get_singer(singer);

        clear_link();
        get_music(singer);
        start_play(); // 内部会设置 g_start_flag = 1
        g_suspend_flag = 0;
        return;
    }

    /* 3. 构造 URL 并发送给 mplayer */
    char path[256] = {0};
    strcat(path, URL);
    strcat(path, music);

    char cmd[256] = {0};
    sprintf(cmd, "loadfile %s\n", path);
    write_fifo(cmd);

    /* 4. 更新共享内存中的当前歌曲名（去掉路径前缀） */
    int i;
    for (i = 0; music[i] && music[i] != '/'; ++i)
        ;
    strcpy(s.cur_music, music + i + (music[i] == '/' ? 1 : 0));
    set_shm(s);

    /* 5. 强制状态为“正在播放”并立即上报服务器 */
    g_start_flag = 1;
    g_suspend_flag = 0;
    send_server(SIGALRM);
}

/*
播放上一首歌
*/
void prior_play()
{
    if (g_start_flag == 0)
        return;

    SHM s;
    get_shm(&s);
    char music[128] = {0};
    find_prior_music(s.cur_music, music);

    char path[256] = {0};
    strcat(path, URL);
    strcat(path, music);

    char cmd[256] = {0};
    sprintf(cmd, "loadfile %s\n", path);

    write_fifo(cmd);

    // 更新共享内存 不然一直卡在第二首切第三首歌
    int i;
    for (i = 0; i < sizeof(music); i++)
    {
        if (music[i] == '/')
            break;
    }
    strcpy(s.cur_music, music + i + 1);
    set_shm(s);

    if (g_suspend_flag == 1)
        g_suspend_flag = 0;
}

/*
循环播放模式
*/
void circle_play()
{
    SHM s;
    get_shm(&s);
    s.mod = SHM_CYCLE;
    set_shm(s);
    printf("---单曲循环---\n");
}

/*
顺序播放模式
*/
void sequence_play()
{
    SHM s;
    get_shm(&s);
    s.mod = SHM_SEQUENCY;
    set_shm(s);
    printf("---顺序播放---\n");
}

/*
播放指定歌手
参数：歌手（const 没必要改名称）
*/
void singer_play(const char *singer)
{
    // 停止播放
    stop_play();

    // 清空链表
    clear_link();

    // 获取歌曲
    get_music(singer);

    // 开始播放
    start_play();
}