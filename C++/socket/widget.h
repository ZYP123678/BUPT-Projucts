#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTcpServer>
#include <QTcpSocket>
#include <qjsonarray.h>
#include<QJsonDocument>
#include<QJsonObject>
#include<QJsonArray>
#include<data_control.h>
#include"define_settings.h"
#include<QException>
#include<QSqlRecord>
#include<QSqlQuery>
#include<QVector>
#include<QList>
#include<database.h>

extern Database da;
QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    void Login(QJsonObject json) const;
    void Register(QJsonObject json) const;
    void SendMessage(QJsonObject json) const;
    void Log_out(QJsonObject json) const;
    void Bag(QJsonObject json) const;
    void Lobby(QJsonObject json) const;
    void Win(QJsonObject json) const;
    void Lost(QJsonObject json) const;
    void Exp(QJsonObject json) const;
    void Userlist(QJsonObject json) const;
    void Exp_2(QJsonObject json) const;
    void Buy(QJsonObject json) const;
    void onClientDisconnected();
    void UpdateUserMedals(int userID) const;
    void onReadyRead() const;
    void onNewConnection();
    ~Widget();

private:
    Ui::Widget *ui;
    QTcpServer *server;
    QTcpSocket *socket;
    QList<QTcpSocket *> socketList;//存放所有客户端socket
    int socketID = 0;
    void client_connect();
    void read_data();
};
#endif // WIDGET_H
