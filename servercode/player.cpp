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
    ��������������
*/
void Player::player_update_list(struct bufferevent *bev, Json::Value &v, Server *ser)
{
    auto it = info->begin();

    // �������� ����豸���� ���������ҷ��͸�������
    for (; it != info->end(); it++)
    {
        if (it->deviceid == v["deviceid"].asString()) // �ҵ��˶�Ӧ�Ľ��
        {
            std::cout << "�豸�Ѿ�����,����������Ϣ..." << std::endl;
            it->cur_music = v["cur_music"].asString();
            it->volume = v["volume"].asInt();
            it->mode = v["mode"].asInt();
            it->d_time = time(NULL);

            if (it->a_bev) // ����Ϊ���ֵ ��Ϊ��
            {
                std::cout << "APP����,����ת����APP..." << std::endl;
                ser->server_send_data(it->a_bev, v);
            }

            return;
        }
    }

    // ����豸������ �½����
    PlayerInfo p;
    p.deviceid = v["deviceid"].asString();
    p.cur_music = v["cur_music"].asString();
    p.volume = v["volume"].asInt();
    p.mode = v["mode"].asInt();
    p.d_time = time(NULL);
    p.d_bev = bev;
    p.a_bev = NULL; // Ҫ����Ϊ�� ��Ϊa_bev��ջ�ռ� �ḳ���ֵ ǰ��ڶ��α�������ʾ��Ϊ��

    info->push_back(p);

    std::cout << "��һ���ϱ�����,����Ѿ�����" << std::endl;
}

/*
    APP��������
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
    ��ʱ��ÿ2���ӱ��������ϱ�
*/
void Player::player_traverse_list()
{
    debug("��ʱ���¼�: ��������");

    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (time(NULL) - it->d_time > 6) // ��Ϊ����2���ϴ�һ������ �������3��Ҳ����6�� ����Ϊ��������
        {
            info->erase(it); // ɾ�����
        }

        if (it->a_bev)
        {
            if (time(NULL) - it->a_time > 6) // ����������
            {
                it->a_bev = NULL;
            }
        }
    }
}

/*
    ��ӡ��Ϣ
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
                debug("APP���� ��������ת���ɹ�");
            }
            else
            {
                debug("APP������ �������ֲ�ת��");
            }
            break;
        }
    }
}

/*
    APP�´�ָ�������ָ��
*/
void Player::player_option(Server *ser, struct bufferevent *bev, Json::Value &v)
{
    // �ж������Ƿ�����
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->a_bev == bev)
        {
            ser->server_send_data(it->d_bev, v);
            debug("��������,ָ��ͳɹ�");
            return;
        }
    }

    // ���䲻����
    Json::Value value;
    std::string cmd = v["cmd"].asString();
    cmd += "_reply";
    value["cmd"] = cmd;
    value["result"] = "offline";

    ser->server_send_data(bev, value);

    debug("���䲻���� ת��ʧ��");
}

/*
    ����ظ����ݸ�APP
*/
void Player::player_reply_option(Server *ser, struct bufferevent *bev, Json::Value &v)
{
    // �ж�APP�Ƿ�����
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->d_bev == bev)
        {
            if (it->a_bev)
            {
                ser->server_send_data(it->a_bev, v);
                debug("APP����,���ݷ��ͳɹ�");
                break;
            }
        }
    }
}

/*
    �ж�APP/��������
*/
void Player::player_offline(struct bufferevent *bev)
{
    for (auto it = info->begin(); it != info->end(); it++)
    {
        if (it->d_bev == bev)
        {
            debug("�����쳣���ߴ���");
            info->erase(it);
            break;
        }

        if (it->a_bev == bev)
        {
            debug("APP�쳣���ߴ���");
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
            debug("APP�������ߴ���");
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