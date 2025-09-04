#ifndef PLAYER_H
#define PLAYER_H

#include <list>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include <event.h>
#include <jsoncpp/json/json.h>
#include <event2/bufferevent.h>
#include <stdlib.h>
#include <sys/types.h>

class Server;

/*
    ��������
*/
struct PlayerInfo
{
    std::string deviceid;  // �豸id��
    std::string appid;     // app id��
    std::string cur_music; // ��ǰ���ŵ�����
    int volume;            // ������С
    int mode;              // ����ģʽ
    time_t d_time;         // ��¼�����ϱ�ʱ��
    time_t a_time;         // ��¼APP�ϱ�ʱ��

    struct bufferevent *d_bev; // ��Ӧ�����¼�
    struct bufferevent *a_bev; // ��ӦAPP�¼�
};

class Player
{
private:
    std::list<PlayerInfo> *info;

public:
    Player();
    ~Player();

    void player_update_list(struct bufferevent *, Json::Value &, Server *);
    void player_app_update_list(struct bufferevent *, Json::Value &);
    void player_traverse_list();
    void debug(const char *);
    void player_upload_music(Server *, struct bufferevent *, Json::Value &);
    void player_option(Server *, struct bufferevent *, Json::Value &);
    void player_reply_option(Server *, struct bufferevent *, Json::Value &);
    void player_offline(struct bufferevent *);
    void player_app_offline(struct bufferevent *);
    void player_get_music(Server *, struct bufferevent *, Json::Value &);
};

#endif