#include "widget.h"
#include "ui_widget.h"

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);

    this->setWindowTitle("智能音箱系统");

    socket = new QTcpSocket;
    socket->connectToHost(QHostAddress("121.196.244.2"), 10001);

    connect(socket, &QTcpSocket::connected, [this]()
    {
        QMessageBox::information(this, "连接提示", "连接成功");
    });

    connect(socket, &QTcpSocket::disconnected, [this]()
    {
        QMessageBox::warning(this, "连接提示", "网络异常断开");
    });

    connect(socket, &QTcpSocket::readyRead, this, &Widget::server_reply_slot);
}

Widget::~Widget()
{
    delete ui;
}


void Widget::on_regBt_clicked()
{
    // 获取appid和password
    QString appid = ui->appidEdit->text();
    QString password = ui->passEdit->text();

    // 发送给服务器  {"cmd":"app_register", "appid":"3333", "password":"11111"}
    QJsonObject obj;
    obj.insert("cmd", "app_register");
    obj.insert("appid", appid);
    obj.insert("password", password);

    widget_send_data(obj);

}

void Widget::widget_send_data(QJsonObject &obj)
{
    QByteArray sendData;
    QByteArray ba = QJsonDocument(obj).toJson();

    int size = ba.size();
    sendData.insert(0, (const char *)&size, 4);
    sendData.append(ba);

    socket->write(sendData);
}

void Widget::server_reply_slot()
{
    QByteArray recvData;

    server_read_data(recvData);

    QJsonObject obj = QJsonDocument::fromJson(recvData).object();

    QString cmd = obj.value("cmd").toString();

    if (cmd == "app_register_reply")
    {
        app_register_handler(obj);
    }
    else if (cmd == "app_login_reply")
    {
        app_login_handler(obj);
    }
}

void Widget::server_read_data(QByteArray & ba)
{
    char buf[1024] = {0};
    qint64 size = 0;

    while (true)
    {
        size += socket->read(buf + size, sizeof (int) - size);
        if (size >= sizeof (int))
            break;
    }

    int len = *(int *)buf;
    size = 0;
    memset(buf, 0, sizeof (buf));

    while (true)
    {
        size += socket->read(buf + size, len - size);
        if (size >= len)
            break;
    }

    ba.append(buf);

    qDebug() << "收到服务器" << len << "字节 :" << ba;
}

void Widget::app_register_handler(QJsonObject &obj)
{
    QString result = obj.value("result").toString();

    if (result == "success")
    {
        QMessageBox::information(this, "注册提示", "注册成功");
    }
    else if (result == "failure")
    {
        QMessageBox::warning(this, "注册提示", "注册失败");
    }
}

void Widget::app_login_handler(QJsonObject &obj)
{
    QString result = obj.value("result").toString();

    if (result == "not_exist")
    {
        QMessageBox::warning(this, "登陆提示", "用户不存在");
    }
    else if (result == "password_error")
    {
        QMessageBox::warning(this, "登陆提示", "密码错误");
    }
    else if (result == "not_bind")
    {
        QMessageBox::information(this, "登陆提示", "登录成功 未绑定");

        socket->disconnect(SIGNAL(connected()));
        socket->disconnect(SIGNAL(disconnected()));
        socket->disconnect(SIGNAL(readyRead()));

        //创建bind界面
        Bind *m_bind = new Bind(socket, m_appid);

        m_bind->show();

        this->hide();

    }
    else if (result == "bind")
    {
        QMessageBox::information(this, "登陆提示", "登录成功 已绑定");

        QString deviceid = obj.value("deviceid").toString();
        qDebug() << deviceid ;

        socket->disconnect(SIGNAL(connected()));
        socket->disconnect(SIGNAL(disconnected()));
        socket->disconnect(SIGNAL(readyRead()));

        //创建控制界面
        Player *m_player = new Player(socket, m_appid, deviceid);

        m_player->show();
        this->hide();
    }
}

void Widget::on_logBt_clicked()
{
    QString appid = ui->appidEdit->text();
    QString password = ui->passEdit->text();

    this->m_appid = appid;

    QJsonObject obj;
    obj.insert("cmd", "app_login");
    obj.insert("appid", appid);
    obj.insert("password", password);

    widget_send_data(obj);
}
