#ifndef LOBBY_H
#define LOBBY_H

#include <QWidget>
#include <QPointer>
#include <QTcpSocket>
#include <QGraphicsDropShadowEffect>
#include <qjsondocument.h>
#include <QJsonObject>
#include <QStandardItemModel>
#include <QStandardItem>
#include<QJsonDocument>
#include<QByteArray>
#include <QJsonArray>
#include <QRandomGenerator>
#include<QMessageBox>
#include <QPropertyAnimation>
#include<define_settings.h>
#include<qtimer.h>
#include<QTimer>
#include<manager.h>
#include<pet.h>
#include<qtableview.h>
#include <QTableWidgetItem>
#include<spiritdialog.h>
#include <SpiritInfoDialog.h>
#include <QGraphicsDropShadowEffect>
#include <QSoundEffect>
#include <QUrl>

extern Manager manage;
namespace Ui {
class Lobby;
}

class Lobby : public QWidget
{
    Q_OBJECT

public:
    explicit Lobby(QTcpSocket *tcpsocket,QWidget *parent = nullptr);
    void reconnect() const;
    void updateUi();
    void setupUserListView(const QJsonArray &spirits) const;
    void refreshPets();
    void battle(pet *s1, pet *s2);//战斗函数
    void Lost(pet *s1);
    void Win(pet *s1, pet *s2);
    void Draw(pet *s1);
    QString getImagePath(const QString &name, int level) const;//寻找图片路径
    void Userlist(QJsonObject json);
    ~Lobby();

private slots:
    void appendToLog(const QString &message);
    void on_all_2_clicked();//显示所有精灵
    void receiveFromServer();//处理接收信息
    void handlePetSelected(int index);//处理选中的精灵
    void on_tabWidget_tabBarClicked(int index);//选中大厅的页面

    void on_log_out_2_clicked();//登出

    void on_last_2_clicked();//上一只精灵信息
    void on_next_2_clicked();//下一只精灵信息
    void updatePlayerHP(int currentHP, int maxHP);//更新显示己方精灵血量
    void updateEnemyHP(int currentHP, int maxHP);//跟新显示敌方精灵血量
    void on_fight_button_2_clicked();//开始战斗
    void onUserAttack();//己方精灵攻击
    void onEnemyAttack();//敌方精灵攻击
    void checkBattleStatus();//检测是否失败


    void on_fail_button_clicked();

    void on_tab_host_tabBarClicked(int index);

    void on_pushButton_clicked();
    void updateResultPage(const QJsonArray &spirits);//更新失败后需丢弃的精灵列表

    void on_other_users_2_clicked();

    void on_back_lobby_clicked();

    void on_userlistTableWidget_itemDoubleClicked(QTableWidgetItem *item);

    void on_back_lobby_2_clicked();

    void on_store_button_2_clicked();

    void on_buy_clicked();

    void on_recover_button_clicked();

    void handleServerDisconnected();

signals:
    void loggedOut(); // 自定义登出信号

private:


    Ui::Lobby *ui;
    QTcpSocket *socket;
    QStringList spiritInfos;//储存所有精灵信息
    int pet_index = 0;
    QJsonArray spiritsArray;//精灵数组
    pet *s1;//用户选择的出战精灵
    pet *s2;//系统出战精灵
    QTimer *userTimer;
    QTimer *enemyTimer;
    int fight_type = 0;//为0为初始页面，经验场，1为决斗赛
    QPointer<SpiritInfoDialog> spiritDialog;
    QSoundEffect *soundEffect;

};

#endif // LOBBY_H
