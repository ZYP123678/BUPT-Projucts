#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <QTcpSocket>
#include "lobby.h"
#include <QGraphicsDropShadowEffect>
#include <qjsondocument.h>
#include <QJsonObject>
#include<QJsonDocument>
#include<QByteArray>
#include<QMessageBox>
#include<define_settings.h>
#include<qtimer.h>
#include<manager.h>
#include <QSoundEffect>
#include <QUrl>


extern Manager manage;

namespace Ui {
class Login;
}

class Login : public QWidget
{
    Q_OBJECT

public:
    explicit Login(QWidget *parent = nullptr);
    Lobby *lobby = NULL;//游戏大厅窗口指针
    ~Login();

signals:
    void log_Out(); // 新增退出登录信号

public slots:

    //连接服务器，发送登陆信息
    void openLoginPage();
    void receiveFromServer();//处理服务器传回的信息
    void reconnect();//可以接受服务器传来的信息

private slots:
    void on_quit_clicked();

    void on_log_in_clicked();//发送登陆信息

    void on_register_2_clicked();//发送注册信息


private:
    Ui::Login *ui;
    QTcpSocket *socket;
    int flag = 0;//标记socketID是否赋值
    QSoundEffect *soundEffect;
};

#endif // LOGIN_H
