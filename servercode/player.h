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
    在线链表
*/
struct PlayerInfo
{
    std::string deviceid;  // 设备id号
    std::string appid;     // app id号
    std::string cur_music; // 当前播放的音乐
    int volume;            // 音量大小
    int mode;              // 播放模式
    time_t d_time;         // 记录音箱上报时间
    time_t a_time;         // 记录APP上报时间

    struct bufferevent *d_bev; // 对应音箱事件
    struct bufferevent *a_bev; // 对应APP事件
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