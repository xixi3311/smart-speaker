#ifndef SERVER_H
#define SERVER_H

#define IP "172.25.82.12"
#define PORT 10001 // 要看虚拟机代码

#include <event.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <event2/listener.h>
#include <stdlib.h>
#include <time.h>
#include <list>
#include <jsoncpp/json/json.h>
#include <event2/bufferevent.h>
#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include "database.h"
#include "player.h"

#define SERQUENCE 1
#define CIRCLE 2

class Server
{
private:
    DataBase *m_database;      // 数据库对象
    struct event_base *m_base; // 事件集合
    Player *m_p;

public:
    Server();
    ~Server();
    void listen(const char *ip, int port);
    struct event_base *server_get_base();

    void server_read_data(struct bufferevent *, char *);
    void server_send_data(struct bufferevent *, Json::Value &);
    void server_get_music(struct bufferevent *, std::string);
    void server_player_handler(struct bufferevent *, Json::Value &);
    void server_start_timer();
    void server_app_register(struct bufferevent *, Json::Value &);
    void server_login(struct bufferevent *, Json::Value &);
    void server_app_bind(struct bufferevent *, Json::Value &);
    void server_client_offline(struct bufferevent *);
    void server_app_offline(struct bufferevent *);

    static void listener_cb(struct evconnlistener *, evutil_socket_t, struct sockaddr *, int socklen, void *);
    static void read_cb(struct bufferevent *, void *);         // 因为它是以函数值形式调用 必须是静态成员函数
    static void event_cb(struct bufferevent *, short, void *); // 因为它是以函数值形式调用 必须是静态成员函数
    static void timeout_cb(evutil_socket_t , short, void *);
};

#endif