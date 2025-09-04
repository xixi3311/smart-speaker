#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include "bind.h"
#include "player.h"

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();

private slots:
    void on_regBt_clicked();

    void on_logBt_clicked();

private:
    void widget_send_data(QJsonObject &);
    void server_reply_slot();
    void server_read_data(QByteArray &);
    void app_register_handler(QJsonObject &);
    void app_login_handler(QJsonObject &);

private:
    Ui::Widget *ui;
    QTcpSocket *socket;
    QString m_appid;
};
#endif // WIDGET_H
