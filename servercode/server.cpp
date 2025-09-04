#include <iostream>
#include "server.h"

Server::Server()
{
    // ��ʼ���¼�����
    m_base = event_base_new();

    // ��ʼ�����ݿ� ���������ݿ���� ������
    m_database = new DataBase;
    if (!m_database->database_init_table())
    {
        std::cout << "���ݿ��ʼ��ʧ��" << std::endl;
        exit(1);
    }

    // ����player���� ���ڴ���APP������֮��Ľ���
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
    ���������������豸����
*/
void Server::listen(const char *ip, int port)
{
    struct sockaddr_in server_info;
    int len = sizeof(server_info);
    memset(&server_info, 0, len);
    server_info.sin_family = AF_INET;
    server_info.sin_port = htons(port);
    server_info.sin_addr.s_addr = inet_addr(ip);

    // ������������ arg����
    struct evconnlistener *listener = evconnlistener_new_bind(m_base, listener_cb, this,
                                                              LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 5, (struct sockaddr *)&server_info, len);
    if (NULL == listener)
    {
        std::cerr << "bind error: " << strerror(errno) << std::endl;
        return;
    }

    std::cout << "Server listening on " << ip << ":" << port << std::endl;

    // ������ʱ��������  ��ѭ��
    server_start_timer();

    // ��������
    // event_base_dispatch(m_base); // ��ѭ��

    printf("2\n");

    // �ͷŶ���
    evconnlistener_free(listener);
    event_base_free(m_base);
}

// һ���пͻ��˷������������󣬾ͻᴥ���ú���
void Server::listener_cb(struct evconnlistener *l, evutil_socket_t fd, struct sockaddr *c, int socklen, void *arg)
{
    struct sockaddr_in *client_info = (struct sockaddr_in *)c;
    // struct event_base *base = (struct event_base *)arg; // ������ǿת
    Server *s = (Server *)arg;
    struct event_base *base = s->server_get_base();

    std::cout << "[new client connect] ";
    std::cout << inet_ntoa(client_info->sin_addr) << ":";
    std::cout << client_info->sin_port << std::endl;

    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    // �����и�ϸ�� bufferevent_socket_new ��event_base *baseû���� ��Ϊlistener_cb��
    // ��̬��Ա���� ����ֱ�����Ǹ�Server�����this->m_base; ����arg����
    // argָ��Server���� ������Ϊm_base��˽�г�Ա ������arg->m_base

    if (NULL == bev)
    {
        std::cout << "bufferevent_socket_new erro" << std::endl;
        return;
    }

    bufferevent_setcb(bev, read_cb, NULL, event_cb, s);
    bufferevent_enable(bev, EV_READ);
}

// �пͻ��˷����ݹ��� �ᴥ���ú���
void Server::read_cb(struct bufferevent *bev, void *ctx)
{
    Server *s = (Server *)ctx;
    char buf[1024] = {0};
    s->server_read_data(bev, buf); // server_read_data�ͱ����ͨ��Ա����

    // ����json
    Json::Reader reader; // ���ڽ����Ķ���
    Json::Value value;   // ��Ž����Ľ��

    if (!reader.parse(buf, value))
    {
        std::cout << "[json parse erro]" << std::endl;
        return;
    }

    if (value["cmd"] == "get_music_list") // ��ȡ��������
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
    ��ȡ��������
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
        // ���������ͨ�ļ� ����ѭ��
        if (d->d_type != DT_REG)
            continue;

        // ����ļ�����MP3�ļ� ����ѭ��
        if (!strstr(d->d_name, ".mp3"))
            continue;

        char name[128] = {0};
        sprintf(name, "%s/%s", s.c_str(), d->d_name);
        l.push_back(name);
    }

    // ���ѡȡ5�׸�
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
    ��������
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
    char buf[8] = {0}; // ������
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
    if (v["cmd"] == "info") // �����ϱ�����
    {
        m_p->player_update_list(bev, v, this);
    }
    else if (v["cmd"] == "app_info") // app�ϱ�����
    {
        m_p->player_app_update_list(bev, v);
    }
    else if (v["cmd"] == "upload_music") // �����ϱ���������
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
        m_p->player_option(this, bev, v); // APP����ת������
    }
    else if (v["cmd"] == "app_start_reply" || v["cmd"] == "app_stop_reply" ||
             v["cmd"] == "app_suspend_reply" || v["cmd"] == "app_continue_reply" ||
             v["cmd"] == "app_prior_reply" || v["cmd"] == "app_next_reply" ||
             v["cmd"] == "app_voice_up_reply" || v["cmd"] == "app_voice_down_reply" ||
             v["cmd"] == "app_circle_reply" || v["cmd"] == "app_sequence_reply")
    {
        m_p->player_reply_option(this, bev, v); // ��������ת����APP
    }
}

void Server::server_start_timer()
{
    struct event tv; // ��ʱ���¼�
    struct timeval t;

    if (event_assign(&tv, m_base, -1, EV_PERSIST, timeout_cb, m_p) == -1)
    {
        std::cout << "event_assign" << std::endl;
        return;
    }

    evutil_timerclear(&t);
    t.tv_sec = 2; // 2��ִ��һ��
    event_add(&tv, &t);

    event_base_dispatch(m_base); // ��ѭ��
}

void Server::timeout_cb(evutil_socket_t fd, short s, void *arg)
{
    Player *p = (Player *)arg;

    p->player_traverse_list();
}

void Server::server_app_register(struct bufferevent *bev, Json::Value &v)
{
    Json::Value val;

    // �������ݿ�
    m_database->database_connect();

    std::string appid = v["appid"].asString();
    std::string password = v["password"].asString();
    // �ж����ݿ�
    if (m_database->database_user_exist(appid)) // �û�����
    {
        val["result"] = "failure";
        std::cout << "[�û����� ע��ʧ��]" << std::endl;
    }
    else // ��������� �޸����ݿ�
    {
        m_database->database_add_user(appid, password);

        val["result"] = "success";
        std::cout << "[�û�ע��ɹ�]" << std::endl;
    }

    // �Ͽ����ݿ�
    m_database->database_disconnect();

    // �ظ�
    val["cmd"] = "app_register_reply";

    server_send_data(bev, val);
}

/*
    �����������¼
*/
void Server::server_login(struct bufferevent *bev, Json::Value &v)
{
    Json::Value val;

    // �������ݿ�
    m_database->database_connect();
    std::string appid = v["appid"].asString();
    std::string password = v["password"].asString();
    std::string deviceid;

    do
    {
        // �ж��û��Ƿ����
        if (!m_database->database_user_exist(appid))
        {
            val["result"] = "not_exist";
            break;
        }

        // �ж������Ƿ���ȷ
        if (!m_database->database_password_correct(appid, password))
        {
            val["result"] = "password_error";
            break;
        }

        // �Ƿ��
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

    // �Ͽ����ݿ�
    m_database->database_disconnect();

    // ����APP
    val["cmd"] = "app_login_reply";

    server_send_data(bev, val);
}

/*
    APP���豸��
*/
void Server::server_app_bind(struct bufferevent *bev, Json::Value &v)
{
    std::string appid = v["appid"].asString();
    std::string deviceid = v["deviceid"].asString();

    // �������ݿ�
    m_database->database_connect();

    // �޸����ݿ�
    m_database->database_bind_user(appid, deviceid);

    // �ر����ݿ�
    m_database->database_disconnect();

    // ��������
    Json::Value val;
    val["cmd"] = "app_bind_reply";
    val["result"] = "success";

    server_send_data(bev, val);
}

void Server::server_app_offline(struct bufferevent *bev)
{
    m_p->player_app_offline(bev);
}