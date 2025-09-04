#ifndef PLAYER_H
#define PLAYER_H

#include <QWidget>
#include <QTcpSocket>
#include <QMessageBox>
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QTimer>
#include <QCloseEvent>

namespace Ui {
class Player;
}

#define SEQUENCE    1
#define CIRCLE      2

class Player : public QWidget
{
    Q_OBJECT

public:
    explicit Player(QTcpSocket *, QString , QString ,QWidget *parent = nullptr);
    ~Player();

private slots:
    void server_reply_slot();
    void timeout_slot();

    void on_playerBt_clicked();

    void on_priorBt_clicked();

    void on_nextBt_clicked();

    void on_VdowmBt_clicked();

    void on_VupBt_clicked();

    void on_circleButton_clicked();

    void on_sequenceBt_clicked();

private:
    void player_send_data(QJsonObject &);
    void player_recv_data(QByteArray &);
    void player_update_info(QJsonObject &);
    void player_update_music(QJsonObject &);
    void player_get_music();
    void player_start_reply(QJsonObject &);
    void player_suspend_reply(QJsonObject &);
    void player_continue_reply(QJsonObject &);
    void player_next_reply(QJsonObject &);
    void player_prior_reply(QJsonObject &);
    void player_voice_reply(QJsonObject &);
    void player_mode_reply(QJsonObject &);
    void closeEvent(QCloseEvent *);

private:
    Ui::Player *ui;
    QString appid;
    QTcpSocket *socket;
    QTimer *timer;
    QString deviceid;
    quint8 flag;
    quint8 start_flag;
    quint8 suspend_flag;
};

#endif // PLAYER_H
