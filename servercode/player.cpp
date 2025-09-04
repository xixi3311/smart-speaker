#include "player.h"
#include "server.h"

Player::Player()
{
    info = new std::list<PlayerInfo>;
}

Player::~Player()
{
    if (info)
        delete info;
}

/*
    服务器更新链表
*/
void Player::player_update_list(struct bufferevent *bev, Json::Value &v, Server *ser)
{
    auto it = info->begin();

    // 遍历链表 如果设备存在 更新链表并且发送给服务器
    for (; it != info->end(); it++)
    {
        if (it->deviceid == v["deviceid"].asString()) // 找到了对应的结点
        {
            std::cout << "设备已经存在,更新链表信息..." << std::endl;
            it->cur_music = v["cur_music"].asString();
            it->volume = v["volume"].asInt();
            it->mode = v["mode"].asInt();
            it->d_time = time(NULL);

            if (it->a_bev) // 发现为随机值 不为空
            {
                std::cout << "APP在线,数据转发给APP..." << std::endl;
                ser->server_send_data(it->a_bev, v);
            }

            return;
        }
    }

    // 如果设备不存在 新建结点
    PlayerInfo p;
    p.deviceid = v["deviceid"].asString();
    p.cur_music = v["cur_music"].asString();
    p.volume = v["volume"].asInt();
    p.mode = v["mode"].asInt();
    p.d_time = time(NULL);
    p.d_bev = bev;
    p.a_bev = NULL; // 要设置为空 因为a_bev是栈空间 会赋随机值 前面第二次遍历会显示不为空

    info->push_back(p);

    std::cout << "第一次上报数据,结点已经建立" << std::endl;
}

/*
    APP更新链表
*/
void Player::player_app_update_list(struct bufferevent *bev, Json::Value &v)
{
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->deviceid == v["deviceid"].asString())
        {
            it->a_time = time(NULL);
            it->appid = v["appid"].asString();
            it->a_bev = bev;
        }
    }
}

/*
    定时器每2秒钟遍历链表上报
*/
void Player::player_traverse_list()
{
    debug("定时器事件: 遍历链表");

    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (time(NULL) - it->d_time > 6) // 因为音箱2秒上传一次数据 如果超过3次也就是6秒 就认为音箱离线
        {
            info->erase(it); // 删掉结点
        }

        if (it->a_bev)
        {
            if (time(NULL) - it->a_time > 6) // 服务器下线
            {
                it->a_bev = NULL;
            }
        }
    }
}

/*
    打印信息
*/
void Player::debug(const char *s)
{
    time_t cur = time(NULL);
    struct tm *t = localtime(&cur);

    std::cout << "[ " << t->tm_hour << " : " << t->tm_min << " : ";
    std::cout << t->tm_sec << " ] " << s << std::endl;
}

void Player::player_upload_music(Server *ser, struct bufferevent *bev, Json::Value &v)
{
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->d_bev == bev)
        {
            if (it->a_bev != NULL)
            {
                ser->server_send_data(it->a_bev, v);
                debug("APP在线 歌曲名字转发成功");
            }
            else
            {
                debug("APP不在线 歌曲名字不转发");
            }
            break;
        }
    }
}

/*
    APP下达指令给音箱指令
*/
void Player::player_option(Server *ser, struct bufferevent *bev, Json::Value &v)
{
    // 判断音箱是否在线
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->a_bev == bev)
        {
            ser->server_send_data(it->d_bev, v);
            debug("音箱在线,指令发送成功");
            return;
        }
    }

    // 音箱不在线
    Json::Value value;
    std::string cmd = v["cmd"].asString();
    cmd += "_reply";
    value["cmd"] = cmd;
    value["result"] = "offline";

    ser->server_send_data(bev, value);

    debug("音箱不在线 转发失败");
}

/*
    音箱回复数据给APP
*/
void Player::player_reply_option(Server *ser, struct bufferevent *bev, Json::Value &v)
{
    // 判断APP是否在线
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->d_bev == bev)
        {
            if (it->a_bev)
            {
                ser->server_send_data(it->a_bev, v);
                debug("APP在线,数据发送成功");
                break;
            }
        }
    }
}

/*
    判断APP/音箱下线
*/
void Player::player_offline(struct bufferevent *bev)
{
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->d_bev == bev)
        {
            debug("音箱异常下线处理");
            info->erase(it);
            break;
        }

        if (it->a_bev == bev)
        {
            debug("APP异常下线处理");
            it->a_bev = NULL;
            break;
        }
    }
}

void Player::player_app_offline(struct bufferevent *bev)
{
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->a_bev == bev)
        {
            debug("APP正常下线处理");
            it->a_bev = NULL;
            break;
        }
    }
}

void Player::player_get_music(Server *ser, struct bufferevent *bev, Json::Value &v)
{
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->a_bev == bev)
        {
            ser->server_send_data(it->d_bev, v);
        }
    }
}