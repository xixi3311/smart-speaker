#include <iostream>
#include "server.h"

Server::Server()
{
    // 初始化事件集合
    m_base = event_base_new();

    // 初始化数据库 （创建数据库对象 创建表）
    m_database = new DataBase;
    if (!m_database->database_init_table())
    {
        std::cout << "数据库初始化失败" << std::endl;
        exit(1);
    }

    // 创建player对象 用于处理APP和音箱之间的交互
    m_p = new Player;
}

Server::~Server()
{
    if (m_database)
        delete m_database;

    if (m_p)
        delete m_p;
}

/*
    服务器监听音箱设备连接
*/
void Server::listen(const char *ip, int port)
{
    struct sockaddr_in server_info;
    int len = sizeof(server_info);
    memset(&server_info, 0, len);
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(port);
    server_info.sin_addr.s_addr = inet_addr(ip);

    // 创建监听对象 arg传给
    struct evconnlistener *listener = evconnlistener_new_bind(m_base, listener_cb, this,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 5, (struct sockaddr *)&server_info, len);
    if (NULL == listener)
    {
        std::cerr << "bind error: " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Server listening on " << ip << ":" << port << std::endl;

    // 启动定时器并监听  死循环
    server_start_timer();

    // 监听集合
    // event_base_dispatch(m_base); // 死循环

    printf("2\n");

    // 释放对象
    evconnlistener_free(listener);
    event_base_free(m_base);
}

// 一旦有客户端发起来连接请求，就会触发该函数
void Server::listener_cb(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr *c, int socklen, void *arg)
{
    struct sockaddr_in *client_info = (struct sockaddr_in *)c;
    // struct event_base *base = (struct event_base *)arg; // 这里做强转
    Server *s = (Server *)arg;
    struct event_base *base = s->server_get_base();

    std::cout << "[new client connect] ";
    std::cout << inet_ntoa(client_info->sin_addr) << ":";
    std::cout << client_info->sin_port << std::endl;

    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    // 这里有个细节 bufferevent_socket_new 的event_base *base没参数 因为listener_cb是
    // 静态成员函数 不能直接用那个Server对象的this->m_base; 用上arg参数
    // arg指向Server对象 但是因为m_base是私有成员 不能用arg->m_base

    if (NULL == bev)
    {
        std::cout << "bufferevent_socket_new erro" << std::endl;
        return;
    }

    bufferevent_setcb(bev, read_cb, NULL, event_cb, s);
    bufferevent_enable(bev, EV_READ);
}

// 有客户端发数据过来 会触发该函数
void Server::read_cb(struct bufferevent *bev, void *ctx)
{
    Server *s = (Server *)ctx;
    char buf[1024] = {0};
    s->server_read_data(bev, buf); // server_read_data就编程普通成员函数

    // 解析json
    Json::Reader reader; // 用于解析的对象
    Json::Value value;   // 存放解析的结果

    if (!reader.parse(buf, value))
    {
        std::cout << "[json parse erro]" << std::endl;
        return;
    }

    if (value["cmd"] == "get_music_list") // 获取音乐数据
    {
        s->server_get_music(bev, value["singer"].asString());
    }
    else if (value["cmd"] == "app_register")
    {
        s->server_app_register(bev, value);
    }
    else if (value["cmd"] == "app_login")
    {
        s->server_login(bev, value);
    }
    else if (value["cmd"] == "app_bind")
    {
        s->server_app_bind(bev, value);
    }
    else if (value["cmd"] == "app_offline")
    {
        s->server_app_offline(bev);
    }
    else
    {
        s->server_player_handler(bev, value);
    }
}

/*
    获取音乐数据
*/
void Server::server_get_music(struct bufferevent *bev, std::string s)
{
    Json::Value val;
    Json::Value arr;
    std::list<std::string> l;
    char path[128] = {0};

    sprintf(path, "/var/www/html/music/%s", s.c_str());
    std::cout << path << std::endl;

    DIR *dir = opendir(path);
    if (NULL == dir)
    {
        perror("opendir");
        return;
    }

    std::cout << "xixi" << std::endl;

    struct dirent *d;
    while ((d = readdir(dir)) != NULL)
    {
        // 如果不是普通文件 继续循环
        if (d->d_type != DT_REG)
            continue;

        // 如果文件不是MP3文件 继续循环
        if (!strstr(d->d_name, ".mp3"))
            continue;

        char name[128] = {0};
        sprintf(name, "%s/%s", s.c_str(), d->d_name);
        l.push_back(name);
    }

    // 随机选取5首歌
    auto it = l.begin();
    srand(time(NULL));
    // std::cout << l.size() << std::endl;
    int count = rand() % (l.size() - 4);
    // std::cout << count << std::endl;
    for (int i = 0; i < count; i++)
    {
        it++;
    }

    for (int i = 0; i < 5 && it != l.end(); i++, it++)
    {
        arr.append(*it);
    }

    val["cmd"] = "reply_music";
    val["music"] = arr;

    server_send_data(bev, val);
}

/*
    发送数据
*/
void Server::server_send_data(struct bufferevent *bev, Json::Value &v)
{
    char msg[1024] = {0};
    std::string SendStr = Json::FastWriter().write(v);
    int len = SendStr.size();
    memcpy(msg, &len, sizeof(int));
    memcpy(msg + sizeof(int), SendStr.c_str(), len);

    if (bufferevent_write(bev, msg, len + sizeof(int)) == -1)
    {
        std::cout << "bufferevent_write erro" << std::endl;
    }
}

/*

*/
void Server::server_read_data(struct bufferevent *bev, char *msg)
{
    char buf[8] = {0}; // 读长度
    size_t size = 0;

    while (true)
    {
        size += bufferevent_read(bev, buf + size, 4 - size);
        if (size >= 4)
            break;
    }

    int len = *(int *)buf;
    size = 0;
    while (true)
    {
        size += bufferevent_read(bev, msg + size, len - size);
        if (size >= len)
            break;
    }

    std::cout << "--len--" << len << "MSG" << msg << std::endl;
}

void Server::event_cb(struct bufferevent *bev, short what, void *ctx)
{
    Server *ser = (Server *)ctx;
    if (what & BEV_EVENT_EOF)
    {
        ser->server_client_offline(bev);
    }
}

void Server::server_client_offline(struct bufferevent *bev)
{
    m_p->player_offline(bev);
}

struct event_base *Server::server_get_base()
{
    return m_base;
}

void Server::server_player_handler(struct bufferevent *bev, Json::Value &v)
{
    if (v["cmd"] == "info") // 音箱上报数据
    {
        m_p->player_update_list(bev, v, this);
    }
    else if (v["cmd"] == "app_info") // app上报数据
    {
        m_p->player_app_update_list(bev, v);
    }
    else if (v["cmd"] == "upload_music") // 音箱上报歌曲名字
    {
        m_p->player_upload_music(this, bev, v);
    }
    else if (v["cmd"] == "app_get_music") 
    {
        m_p->player_get_music(this, bev, v);
    }
    else if (v["cmd"] == "app_start" || v["cmd"] == "app_stop" ||
             v["cmd"] == "app_suspend" || v["cmd"] == "app_continue" ||
             v["cmd"] == "app_prior" || v["cmd"] == "app_next" ||
             v["cmd"] == "app_voice_down" || v["cmd"] == "app_voice_up" ||
             v["cmd"] == "app_circle" || v["cmd"] == "app_sequence")
    {
        m_p->player_option(this, bev, v); // APP数据转发音箱
    }
    else if (v["cmd"] == "app_start_reply" || v["cmd"] == "app_stop_reply" ||
             v["cmd"] == "app_suspend_reply" || v["cmd"] == "app_continue_reply" ||
             v["cmd"] == "app_prior_reply" || v["cmd"] == "app_next_reply" ||
             v["cmd"] == "app_voice_up_reply" || v["cmd"] == "app_voice_down_reply" ||
             v["cmd"] == "app_circle_reply" || v["cmd"] == "app_sequence_reply")
    {
        m_p->player_reply_option(this, bev, v); // 音箱数据转发给APP
    }
}

void Server::server_start_timer()
{
    struct event tv; // 定时器事件
    struct timeval t;

    if (event_assign(&tv, m_base, -1, EV_PERSIST, timeout_cb, m_p) == -1)
    {
        std::cout << "event_assign" << std::endl;
        return;
    }

    evutil_timerclear(&t);
    t.tv_sec = 2; // 2秒执行一次
    event_add(&tv, &t);

    event_base_dispatch(m_base); // 死循环
}

void Server::timeout_cb(evutil_socket_t fd, short s, void *arg)
{
    Player *p = (Player *)arg;

    p->player_traverse_list();
}

void Server::server_app_register(struct bufferevent *bev, Json::Value &v)
{
    Json::Value val;

    // 连接数据库
    m_database->database_connect();

    std::string appid = v["appid"].asString();
    std::string password = v["password"].asString();
    // 判断数据库
    if (m_database->database_user_exist(appid)) // 用户存在
    {
        val["result"] = "failure";
        std::cout << "[用户存在 注册失败]" << std::endl;
    }
    else // 如果不存在 修改数据库
    {
        m_database->database_add_user(appid, password);

        val["result"] = "success";
        std::cout << "[用户注册成功]" << std::endl;
    }

    // 断开数据库
    m_database->database_disconnect();

    // 回复
    val["cmd"] = "app_register_reply";

    server_send_data(bev, val);
}

/*
    服务器处理登录
*/
void Server::server_login(struct bufferevent *bev, Json::Value &v)
{
    Json::Value val;

    // 连接数据库
    m_database->database_connect();
    std::string appid = v["appid"].asString();
    std::string password = v["password"].asString();
    std::string deviceid;

    do
    {
        // 判断用户是否存在
        if (!m_database->database_user_exist(appid))
        {
            val["result"] = "not_exist";
            break;
        }

        // 判断密码是否正确
        if (!m_database->database_password_correct(appid, password))
        {
            val["result"] = "password_error";
            break;
        }

        // 是否绑定
        if (m_database->database_user_bind(appid, deviceid))
        {
            val["result"] = "bind";
            val["deviceid"] = deviceid;
        }
        else
        {
            val["result"] = "not_bind";
        }
    } while (0);

    // 断开数据库
    m_database->database_disconnect();

    // 返回APP
    val["cmd"] = "app_login_reply";

    server_send_data(bev, val);
}

/*
    APP绑定设备号
*/
void Server::server_app_bind(struct bufferevent *bev, Json::Value &v)
{
    std::string appid = v["appid"].asString();
    std::string deviceid = v["deviceid"].asString();

    // 连接数据库
    m_database->database_connect();

    // 修改数据库
    m_database->database_bind_user(appid, deviceid);

    // 关闭数据库
    m_database->database_disconnect();

    // 返回数据
    Json::Value val;
    val["cmd"] = "app_bind_reply";
    val["result"] = "success";

    server_send_data(bev, val);
}

void Server::server_app_offline(struct bufferevent *bev)
{
    m_p->player_app_offline(bev);
}